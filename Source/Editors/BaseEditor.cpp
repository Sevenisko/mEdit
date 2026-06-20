#include <Editors/BaseEditor.h>
#include <EditorApplication.h>
#include <Utils.h>
#include <Hacks.hpp>
#include <Utils/WinUtils.h>

#include <imgui_internal.h>
#include <Utils/ImGuiUtils.h>
#include <imgui_impl_dx8.h>
#include <imgui_impl_dinput.h>
#include <imgui_impl_win32.h>

#include <IO/BinaryReader.hpp>
#include <IO/ChunkReader.hpp>
#include <Cache.h>

#include <I3D/I3D_visual.h>
#include <I3D/I3D_light.h>
#include <I3D/I3D_sound.h>
#include <I3D/I3D_model.h>
#include <I3D/Visuals/I3D_lit_object.h>

#include <MinHook.h>

BaseEditor* g_ActiveEditor = nullptr;

std::map<I3D_sound*, std::string> g_SoundsMap;
std::map<I3D_model*, std::string> g_ModelsMap;

// Editor log system (shared)
struct LogEntry {
    std::string message;
    ImVec4 color;
};
static std::vector<LogEntry> g_LogEntries;
static bool g_LogAutoScroll = true;

void editorLog(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    g_LogEntries.push_back({buf, ImVec4(0.85f, 0.85f, 0.85f, 1.0f)});
    debugPrintf("%s", buf);
}

void editorLogWarning(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    g_LogEntries.push_back({buf, ImVec4(1.0f, 0.85f, 0.3f, 1.0f)});
    debugPrintf("[WARN] %s", buf);
}

void editorLogError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    g_LogEntries.push_back({buf, ImVec4(1.0f, 0.35f, 0.35f, 1.0f)});
    debugPrintf("[ERR] %s", buf);
}

static WNDPROC g_OriginalWndProc = NULL;
static WNDPROC g_OriginalChildWndProc = NULL;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK BaseEditorChildWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
    case WM_DESTROY:
        if(g_OriginalChildWndProc) {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)g_OriginalChildWndProc);
            g_OriginalChildWndProc = NULL;
        }
        break;
    }

    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

    return CallWindowProc(g_OriginalChildWndProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK BaseEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case BCMD_FILE_OPEN:
            if(g_ActiveEditor) g_ActiveEditor->OnFileOpen();
            return 0;
        case BCMD_FILE_SAVE:
            // Handled by subclass via OnMenuCommand
            if(g_ActiveEditor) g_ActiveEditor->OnMenuCommand(BCMD_FILE_SAVE);
            return 0;
        case BCMD_FILE_CLOSE:
            if(g_ActiveEditor) g_ActiveEditor->OnFileClose();
            if(g_ActiveEditor) g_ActiveEditor->Shutdown();
            ExitProcess(0);
            return 0;
        default:
            // Dispatch to subclass
            if(g_ActiveEditor) g_ActiveEditor->OnMenuCommand(LOWORD(wParam));
            return 0;
        }
        break;

    case WM_CLOSE: SendMessage(hwnd, WM_COMMAND, BCMD_FILE_CLOSE, 0); return 0;
    case WM_DESTROY:
        if(g_OriginalWndProc) {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
            g_OriginalWndProc = NULL;
        }
        break;
    }

    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

    return CallWindowProc(g_OriginalWndProc, hwnd, msg, wParam, lParam);
}

// LS3DF module handle (shared)
static HMODULE s_ls3df = nullptr;

static void InvalidateImGuiResources() {
    if(!g_ActiveEditor) return;
    ImGui::SetCurrentContext(g_ActiveEditor->GetImGuiContext());
    g_ActiveEditor->ReleaseViewportRT();
    ImGui_ImplDX8_InvalidateDeviceObjects();
}

static void CreateImGuiResources() {
    if(!g_ActiveEditor) return;
    ImGui::SetCurrentContext(g_ActiveEditor->GetImGuiContext());
    ImGui_ImplDX8_CreateDeviceObjects();
    g_ActiveEditor->CreateViewportRT(800, 600);
}

typedef int(__cdecl* dbgPrintf_t)(const char* fmt, ...);
static dbgPrintf_t dbgPrintf_orig = nullptr;

static int dbgPrintf_Hook(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsprintf(buf, fmt, args);
    va_end(args);

    char buf2[512];
    sprintf(buf2, "%s\n", buf);
    OutputDebugStringA(buf2);

    return dbgPrintf_orig(buf);
}

__declspec(naked) void BaseResetDevice_Before() {
    static DWORD address = (int)s_ls3df + 0x6D5EE;

    __asm {
        call InvalidateImGuiResources
        mov eax, address
        jmp eax
    }
}

__declspec(naked) void BaseResetDevice_After() {
    static DWORD address = (int)s_ls3df + 0x6D62C;

    *(bool*)((int)s_ls3df + 0xC59B3) = false;

    __asm {
        call CreateImGuiResources
        mov eax, address
        jmp eax
    }
}

float CalculateHorizontalFOV(float aspectRatio) {
    const float points[][2] = {
        {1.0f, 80.0f},
        {1.25f, 86.0f},
        {1.333f, 90.0f},
        {1.6f, 108.0f},
        {1.777f, 115.0f},
        {2.370f, 135.0f},
        {3.556f, 150.0f}
    };
    const int numPoints = sizeof(points) / sizeof(points[0]);

    if(aspectRatio <= points[0][0]) { return RAD(80.0f); }
    if(aspectRatio >= points[numPoints - 1][0]) { return RAD(150.0f); }

    for(int i = 0; i < numPoints - 1; ++i) {
        if(aspectRatio >= points[i][0] && aspectRatio <= points[i + 1][0]) {
            float x1 = points[i][0], y1 = points[i][1];
            float x2 = points[i + 1][0], y2 = points[i + 1][1];
            return RAD((y1 + (y2 - y1) / (x2 - x1) * (aspectRatio - x1)));
        }
    }

    return RAD(90.0f);
}

float CalculateCameraFOV(float horizontalFOV, float aspectRatio) { return 2.0f * atanf(tanf(horizontalFOV / 2.0f) / aspectRatio); }

// FPS timing globals
static INT64 g_QPCFreq;
static INT32 g_FrameCount = 0;
static INT64 g_FrameStart = 0, g_FrameEnd = 0;
static INT64 g_AverageFPS = 0, g_TicksAccum = 0;
static INT64 g_ElapsedTime, g_OverSleepDuration = 0;
static const INT64 BASE_TARGET_FRAME_TIME = (1000000 / TARGET_FPS_CAP) + 1;
static float g_FramerateUpdateTime = 0;

static INT64 __forceinline ElapsedMicroseconds(INT64 startCount, INT64 endCount) {
    INT64 elapsedMicroseconds = endCount - startCount;
    elapsedMicroseconds *= 1000000;
    elapsedMicroseconds /= g_QPCFreq;
    return elapsedMicroseconds;
}

bool BaseEditor::Init() {
    g_ActiveEditor = this;

    MH_CreateHook((LPVOID)&dbgPrintf, (LPVOID)&dbgPrintf_Hook, (LPVOID*)&dbgPrintf_orig);
    MH_EnableHook(MH_ALL_HOOKS);

    m_IGraph = GetIGraph();
    m_3DDriver = I3DGetDriver();
    m_SoundDriver = ISndGetDriver();

    EditorSettings* settings = g_Editor->GetSettings();

    // Create base menus
    m_MenuBar = CreateMenu();
    m_FileMenu = CreatePopupMenu();

    AppendMenu(m_FileMenu, MF_STRING, BCMD_FILE_OPEN, "Open\tCtrl+O");
    AppendMenu(m_FileMenu, MF_STRING, BCMD_FILE_SAVE, "Save\tCtrl+S");
    EnableMenuItem(m_FileMenu, BCMD_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    AppendMenu(m_FileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(m_FileMenu, MF_STRING, BCMD_FILE_CLOSE, "Exit\tAlt+F4");

    // File menu goes first
    AppendMenu(m_MenuBar, MF_POPUP, (UINT_PTR)m_FileMenu, "File");

    // Let subclass append its menus after File
    OnMenuSetup();

    IGRAPH_INIT_DESC initDesc;
    ZeroMemory(&initDesc, sizeof(IGRAPH_INIT_DESC));

    initDesc.Instance = GetModuleHandleA(NULL);
    initDesc.BPP = 32;
    initDesc.X = 60;
    initDesc.Y = 60;
    initDesc.CurrentAdapter = settings->video.currentAdapter;
    initDesc.Width = initDesc.Rc.right = settings->video.width;
    initDesc.Height = initDesc.Rc.bottom = settings->video.height;
    initDesc.RefreshRate = settings->video.refreshRate;
    if(settings->video.fullscreen) {
        initDesc.Flags |= INITFLAG_FULLSCREEN;
    } else {
        initDesc.Menu = m_MenuBar;
    }
    if(settings->video.vsync) { initDesc.Flags |= INITFLAG_VSYNC; }

    IDirect3D8* d3d = Direct3DCreate8(D3D_SDK_VERSION);
    D3DDISPLAYMODE currentMode;
    HRESULT hr = d3d->GetAdapterDisplayMode(initDesc.CurrentAdapter, &currentMode);
    if(SUCCEEDED(hr)) {
        initDesc.X = (currentMode.Width / 2) - (initDesc.Width / 2);
        initDesc.Y = (currentMode.Height / 2) - (initDesc.Height / 2);
    }
    d3d->Release();

    LS3D_RESULT res = m_IGraph->Init(&initDesc);

    if(I3D_SUCCESS(res)) {
        g_OriginalWndProc = (WNDPROC)SetWindowLongPtr(m_IGraph->GetMainHWND(), GWLP_WNDPROC, (LONG_PTR)BaseEditorWndProc);
        g_OriginalChildWndProc = (WNDPROC)SetWindowLongPtr(m_IGraph->GetChildHWND(), GWLP_WNDPROC, (LONG_PTR)BaseEditorChildWndProc);

        HICON iconBig = CreateIconFromPNG(GetIconPath(), 128, 128);
        HICON icon = CreateIconFromPNG(GetIconPath(), 32, 32);

        SendMessage(m_IGraph->GetMainHWND(), WM_SETICON, ICON_SMALL, (LPARAM)icon);
        SendMessage(m_IGraph->GetMainHWND(), WM_SETICON, ICON_BIG, (LPARAM)iconBig);

        m_IGraph->SetAppName(GetEditorName());
        m_IGraph->ResetRenderProps();
        m_IGraph->Clear(0x3F800000, 1.0, 0);
        m_IGraph->Present();

        res = m_3DDriver->Init(0);

        if(I3D_SUCCESS(res)) {
            m_3DDriver->SetState(RS_ENVMAPPING, 1);
            m_3DDriver->SetState(RS_USESHADOWS, 1);
            m_3DDriver->SetState(RS_FOG, 1);
            m_3DDriver->SetState(RS_DEBUG_INT4, 8);
            m_3DDriver->SetState(RS_DEBUG_INT5, 8);
            m_3DDriver->SetState(RS_DRAW_COL_TESTS, 1);
            m_3DDriver->SetState(RS_SOUND_VOLUME, 0);
            m_3DDriver->SetState(RS_PROFILER_MODE, 0);
            m_3DDriver->SetState(RS_LOD_INDEX, 0);
            m_3DDriver->SetState(RS_ANISO_FILTERING, 1);
            m_3DDriver->SetState(RS_USE_OCCLUSION, 0);
            m_3DDriver->SetState(RS_TEXTUREDITHER, 0);
            m_3DDriver->SetState(RS_DRAWTEXTURES, 1);
        }

        ISND_INIT sndInit;
        sndInit.hWindow = m_IGraph->GetMainHWND();
        sndInit.uBps = 24;
        sndInit.uNumChan = 2;
        sndInit.uMixFreq = 44100;
        m_SoundDriver->Init(&sndInit);

        m_IGraph->AddMapsDir("Maps", false);
        m_IGraph->MouseInit(0);

        s_ls3df = GetModuleHandleA("LS3DF.dll");

        if(s_ls3df) {
            m_D3DDevice = *(IDirect3DDevice8**)((int)s_ls3df + 0x1C597C);
            m_LS3DMouseDevice = *(IDirectInputDevice8A**)((int)s_ls3df + 0x1C595C);
        } else {
            m_D3DDevice = *(IDirect3DDevice8**)(0x101C597C);
            m_LS3DMouseDevice = *(IDirectInputDevice8A**)(0x101C595C);
        }

        m_Camera = (I3D_camera*)m_3DDriver->CreateFrame(FRAME_CAMERA);

        m_LastFrameTime = std::chrono::steady_clock::now();
        m_DeltaTime = 0.0f;

        m_Camera->SetName("EditorCamera");
        float aspectRatio = float(settings->video.width) / settings->video.height;
        m_Camera->SetAspectRatio(aspectRatio);

        float horizontalFOV = CalculateHorizontalFOV(aspectRatio);
        float camFOV = CalculateCameraFOV(horizontalFOV, aspectRatio);

        m_Camera->SetFOV(camFOV);
        m_Camera->SetRange(0.01f, 8000.0f);

        m_ImGuiContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_ImGuiContext);

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Each editor gets its own ini file
        static std::string iniPath;
        iniPath = std::string("mEdit\\") + GetEditorName() + "_gui.ini";
        // Replace spaces with underscores
        for(auto& c : iniPath) { if(c == ' ') c = '_'; }
        io.IniFilename = iniPath.c_str();

        QueryPerformanceFrequency((LARGE_INTEGER*)&g_QPCFreq);
        QueryPerformanceCounter((LARGE_INTEGER*)&g_FrameStart);

        SetupImGui();

        InstallJmpHook((int)s_ls3df + 0x6D5E4, (DWORD)&BaseResetDevice_Before);
        InstallJmpHook((int)s_ls3df + 0x6D603, (DWORD)&BaseResetDevice_After);
        InstallJmpHook((int)s_ls3df + 0x57DD4, (int)s_ls3df + 0x57E74);
        InstallJmpHook((int)s_ls3df + 0x5FB65, (int)s_ls3df + 0x5FBA1);

        ImGui_ImplWin32_Init(m_IGraph->GetMainHWND());
        ImGui_ImplDX8_Init(m_D3DDevice);

        CreateViewportRT(800, 600);

        return true;
    }

    return false;
}

bool BaseEditor::IsInit() const { return m_IGraph && m_IGraph->IsInit(); }

void BaseEditor::ReleaseViewportRT() {
    if(m_ViewportSurface) {
        m_ViewportSurface->Release();
        m_ViewportSurface = nullptr;
    }
    if(m_ViewportDepth) {
        m_ViewportDepth->Release();
        m_ViewportDepth = nullptr;
    }
    if(m_ViewportTexture) {
        m_ViewportTexture->Release();
        m_ViewportTexture = nullptr;
    }
    m_ViewportWidth = 0;
    m_ViewportHeight = 0;
}

void BaseEditor::CreateViewportRT(UINT width, UINT height) {
    ReleaseViewportRT();

    if(width == 0 || height == 0) return;

    HRESULT hr = m_D3DDevice->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_ViewportTexture);
    if(FAILED(hr)) return;

    m_ViewportTexture->GetSurfaceLevel(0, &m_ViewportSurface);

    hr = m_D3DDevice->CreateDepthStencilSurface(width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, &m_ViewportDepth);
    if(FAILED(hr)) {
        ReleaseViewportRT();
        return;
    }

    m_ViewportWidth = width;
    m_ViewportHeight = height;
}

#define LINE_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)

void BaseEditor::DrawBatchedLines(const std::vector<LineVertex>& vertices, const std::vector<WORD>& indices, const S_vector& color, uint8_t alpha) {
    if(vertices.empty()) return;

    bool useIndexing = !indices.empty();
    if(useIndexing && indices.size() % 2 != 0) return;

    float calcAplha = (double)(255 - (unsigned int)alpha) * 0.0039215689;
    float a = calcAplha * 255.0;
    float r = color.x * 255.0;
    float g = color.y * 255.0;
    float b = color.z * 255.0;
    uint32_t d3dColor = (int)b | (((int)g | (((int)r | ((int)a << 8)) << 8)) << 8);

    std::vector<LineVertex> coloredVerts = vertices;
    for(auto& v: coloredVerts) {
        if(v.color == 0) v.color = d3dColor;
    }

    IDirect3DVertexBuffer8* pVB = nullptr;
    UINT vbSize = static_cast<UINT>(coloredVerts.size() * sizeof(LineVertex));
    m_D3DDevice->CreateVertexBuffer(vbSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, LINE_FVF, D3DPOOL_DEFAULT, &pVB);
    if(!pVB) return;

    void* pVBData = nullptr;
    pVB->Lock(0, 0, (BYTE**)&pVBData, D3DLOCK_DISCARD);
    memcpy(pVBData, coloredVerts.data(), vbSize);
    pVB->Unlock();

    IDirect3DIndexBuffer8* pIB = nullptr;
    if(useIndexing) {
        UINT ibSize = static_cast<UINT>(indices.size() * sizeof(WORD));
        m_D3DDevice->CreateIndexBuffer(ibSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &pIB);
        if(pIB) {
            void* pIBData = nullptr;
            pIB->Lock(0, 0, (BYTE**)&pIBData, D3DLOCK_DISCARD);
            memcpy(pIBData, indices.data(), ibSize);
            pIB->Unlock();
            m_D3DDevice->SetIndices(pIB, 0);
        }
    }

    m_D3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
    m_D3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
    m_D3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_D3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    m_D3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_D3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_D3DDevice->SetRenderState(D3DRS_ALPHAREF, TRUE);
    m_D3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_D3DDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    m_D3DDevice->SetTexture(0, nullptr);
    m_D3DDevice->SetStreamSource(0, pVB, sizeof(LineVertex));
    m_D3DDevice->SetVertexShader(LINE_FVF);

    UINT numPrimitives = useIndexing ? static_cast<UINT>(indices.size() / 2) : static_cast<UINT>(coloredVerts.size() / 2);
    if(useIndexing && pIB) {
        m_D3DDevice->DrawIndexedPrimitive(D3DPT_LINELIST, 0, static_cast<UINT>(coloredVerts.size()), 0, numPrimitives);
    } else {
        m_D3DDevice->DrawPrimitive(D3DPT_LINELIST, 0, numPrimitives);
    }

    pVB->Release();
    if(pIB) pIB->Release();
}

void BaseEditor::DrawWireframeBox(const I3D_bbox& bbox, const S_vector& color, uint8_t alpha) {
    S_vector pos[8] = {
        {bbox.min.x, bbox.min.y, bbox.min.z},
        {bbox.max.x, bbox.min.y, bbox.min.z},
        {bbox.max.x, bbox.min.y, bbox.max.z},
        {bbox.min.x, bbox.min.y, bbox.max.z},
        {bbox.min.x, bbox.max.y, bbox.min.z},
        {bbox.max.x, bbox.max.y, bbox.min.z},
        {bbox.max.x, bbox.max.y, bbox.max.z},
        {bbox.min.x, bbox.max.y, bbox.max.z}
    };

    std::vector<LineVertex> vertices(8);
    for(int i = 0; i < 8; ++i) {
        vertices[i] = {pos[i].x, pos[i].y, pos[i].z, 0};
    }

    std::vector<WORD> indices = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };

    DrawBatchedLines(vertices, indices, color, alpha);
}

void BaseEditor::DrawWireframeCone(const S_vector& pos, const S_vector& dir, float radius, float height, const S_vector& color, uint8_t alpha, int segments) {
    std::vector<S_vector> posVerts(segments + 1);

    S_vector position = pos;

    S_vector nDir;
    nDir.SetNormalized(dir);

    posVerts[0] = position;

    S_vector up(0.0f, 1.0f, 0.0f);
    if(fabs(nDir.Dot(up)) > 0.99f) up = S_vector(1.0f, 0.0f, 0.0f);

    S_vector right = nDir.Cross(up);
    right.SetNormalized(right);
    S_vector perp = nDir.Cross(right);
    perp.SetNormalized(perp);

    S_vector baseCenter = position + nDir * height;

    for(int i = 0; i < segments; ++i) {
        float angle = (float)i / (float)segments * 2.0f * 3.14159265f;
        S_vector offset = right * cosf(angle) * radius + perp * sinf(angle) * radius;
        posVerts[i + 1] = baseCenter + offset;
    }

    std::vector<LineVertex> verts;
    std::vector<WORD> indices;
    verts.resize(segments + 1);
    for(int i = 0; i <= segments; ++i) {
        verts[i] = {posVerts[i].x, posVerts[i].y, posVerts[i].z, 0};
    }

    for(int i = 0; i < segments; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
    }

    for(int i = 0; i < segments; ++i) {
        indices.push_back(i + 1);
        indices.push_back((i + 1) % segments + 1);
    }

    DrawBatchedLines(verts, indices, color, alpha);
}

void BaseEditor::DrawWireframeCylinder(const S_vector& basePos, float radius, float height, const S_vector& color, uint32_t alpha, int segments) {
    std::vector<LineVertex> verts;
    std::vector<WORD> idx;
    WORD off = 0;

    for(int ring = 0; ring < 2; ++ring) {
        float y = basePos.y + (ring == 0 ? 0.0f : height);
        for(int i = 0; i < segments; ++i) {
            float angle = (float)i / (float)segments * 2.0f * 3.14159265f;
            float x = basePos.x + cosf(angle) * radius;
            float z = basePos.z + sinf(angle) * radius;
            verts.push_back({x, y, z, 0});
        }
    }

    for(int i = 0; i < segments; ++i) {
        idx.push_back(i);
        idx.push_back((i + 1) % segments);
    }

    for(int i = 0; i < segments; ++i) {
        idx.push_back(segments + i);
        idx.push_back(segments + (i + 1) % segments);
    }

    for(int i = 0; i < segments; ++i) {
        idx.push_back(i);
        idx.push_back(segments + i);
    }

    DrawBatchedLines(verts, idx, color, (uint8_t)alpha);
}

void BaseEditor::DrawWireframeFrustum(const S_vector& pos,
                                      const S_vector& dir,
                                      float widthTop,
                                      float heightTop,
                                      float widthBottom,
                                      float heightBottom,
                                      float height,
                                      const S_vector& color,
                                      uint8_t alpha) {
    S_vector nDir;
    nDir.SetNormalized(dir);

    S_vector up(0, 1, 0);
    if(fabs(nDir.Dot(up)) > 0.99f) up = S_vector(1, 0, 0);

    S_vector right = nDir.Cross(up);
    right.SetNormalized(right);
    S_vector perp = nDir.Cross(right);
    perp.SetNormalized(perp);

    S_vector basePos = pos; // mutable copy (S_vector::operator+ is non-const)
    S_vector topCenter = basePos + nDir * height;

    S_vector bottom[4] = {
        basePos + right * (-widthBottom * 0.5f) + perp * (-heightBottom * 0.5f),
        basePos + right * (widthBottom * 0.5f) + perp * (-heightBottom * 0.5f),
        basePos + right * (widthBottom * 0.5f) + perp * (heightBottom * 0.5f),
        basePos + right * (-widthBottom * 0.5f) + perp * (heightBottom * 0.5f),
    };

    S_vector top[4] = {
        topCenter + right * (-widthTop * 0.5f) + perp * (-heightTop * 0.5f),
        topCenter + right * (widthTop * 0.5f) + perp * (-heightTop * 0.5f),
        topCenter + right * (widthTop * 0.5f) + perp * (heightTop * 0.5f),
        topCenter + right * (-widthTop * 0.5f) + perp * (heightTop * 0.5f),
    };

    std::vector<LineVertex> verts(8);
    for(int i = 0; i < 4; ++i) {
        verts[i] = {bottom[i].x, bottom[i].y, bottom[i].z, 0};
        verts[4 + i] = {top[i].x, top[i].y, top[i].z, 0};
    }

    std::vector<WORD> indices = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };

    DrawBatchedLines(verts, indices, color, alpha);
}

void BaseEditor::DrawProgress(const std::string& title) {
    m_IGraph->ResetRenderProps();
    m_IGraph->Clear(0xFF000000, 1.0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    m_3DDriver->DrawText2D((m_IGraph->Scrn_sx() / 2) - ((title.length() / 2) * 16), (m_IGraph->Scrn_sy() / 2) - 8, title.c_str(), 0, 1.0f);
    m_IGraph->Present();
}

bool BaseEditor::LoadScene(const std::string& scenePath) {
    Clear();

    m_Scene = (I3D_scene*)m_3DDriver->CreateFrame(FRAME_SCENE);
    m_PrimarySector = m_Scene->GetPrimarySector();
    m_BackdropSector = m_Scene->GetBackdropSector();

    m_Scene->SetActiveCamera(m_Camera);

    m_CameraPos = {0, 0, 0};
    m_CameraRot = {0, 0};

    // Save original render target and redirect to viewport texture
    IDirect3DSurface8* origRT = nullptr;
    IDirect3DSurface8* origDS = nullptr;
    if(m_ViewportSurface) {
        m_D3DDevice->GetRenderTarget(&origRT);
        m_D3DDevice->GetDepthStencilSurface(&origDS);
        m_D3DDevice->SetRenderTarget(m_ViewportSurface, m_ViewportDepth);
        D3DVIEWPORT8 vp = {0, 0, m_ViewportWidth, m_ViewportHeight, 0.0f, 1.0f};
        m_D3DDevice->SetViewport(&vp);
        if(m_Scene) { m_Scene->SetViewport(0, 0, m_ViewportWidth, m_ViewportHeight); }
        if(m_Camera) {
            float vpAspect = (float)m_ViewportWidth / (float)m_ViewportHeight;
            m_Camera->SetAspectRatio(vpAspect);
            float hfov = CalculateHorizontalFOV(vpAspect);
            m_Camera->SetFOV(CalculateCameraFOV(hfov, vpAspect));
        }
    }

    m_IGraph->Clear(0xFF000000, 1.0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);

    // Restore original render target
    if(origRT || origDS) {
        m_D3DDevice->SetRenderTarget(origRT, origDS);
        if(origRT) origRT->Release();
        if(origDS) origDS->Release();

        D3DVIEWPORT8 bbVP = {0, 0, (DWORD)m_IGraph->Scrn_sx(), (DWORD)m_IGraph->Scrn_sy(), 0.0f, 1.0f};
        m_D3DDevice->SetViewport(&bbVP);
    }

    editorLog("Loading: scene.4ds");
    LS3D_RESULT res = m_Scene->Open((scenePath + "\\scene.i3d").c_str(), 0, nullptr, nullptr);

    if(I3D_FAIL(res)) {
        editorLogError("Failed to load %s!", (scenePath + "\\scene.4ds").c_str());
        return false;
    }

    m_SceneLoaded = true;
    return true;
}

bool BaseEditor::LoadScene2Bin(const std::string& fileName) {
    BinaryReader reader(fileName);
    if(!reader.IsOpen()) {
        editorLogError("Failed to load %s!", fileName.c_str());
        return false;
    }

    ChunkReader chunk(&reader);
    if(++chunk == CT_BASECHUNK) {
        while(chunk) {
            ChunkType type = ++chunk;
            switch((E_CHUNK_TYPE)type) {
            case CT_VERSION: {
                chunk.Read<uint16_t>(); // major
                chunk.Read<uint16_t>(); // minor
                chunk.ReadNullTerminatedString(); // copyright
                chunk.Read<uint8_t>();
            } break;

            case CT_CAMERA_RANGE: {
                float range = chunk.Read<float>();
                m_Camera->SetRange(0.2f, range);
            } break;

            case CT_SCENE_CLIPPING_RANGE: {
                float nearRange = chunk.Read<float>();
                float farRange = chunk.Read<float>();
                m_Scene->SetBackdropRange(nearRange, farRange);
            } break;

            case CT_MODIFICATIONS: {
                while(chunk) {
                    auto modType = ++chunk;
                    if(modType == CT_MODIFICATION) {
                        I3D_frame* frame = nullptr;
                        I3D_FRAME_TYPE frameType = FRAME_NULL;
                        I3D_VISUAL_TYPE visualType = VISUAL_OBJECT;
                        I3D_frame* parentFrame = nullptr;
                        S_vector pos, scale;
                        bool posOK = false, scaleOK = false;
                        S_quat rot;
                        bool rotOK = false;
                        bool isHidden = false;
                        bool created = false;

                        while(chunk) {
                            ChunkType t = ++chunk;
                            switch(t) {
                            case CT_FRAME_TYPE: {
                                frameType = static_cast<I3D_FRAME_TYPE>(chunk.Read<uint32_t>());
                            } break;

                            case CT_FRAME_SUBTYPE: {
                                visualType = static_cast<I3D_VISUAL_TYPE>(chunk.Read<uint32_t>());
                            } break;

                            case CT_FRAME_FLAGS: {
                                chunk.Read<uint32_t>();
                            } break;

                            case CT_NAME: {
                                auto name = chunk.ReadNullTerminatedString();
                                frame = m_Scene->FindFrame(name.c_str(), ENUMF_ALL);
                                if(!frame) {
                                    if(frameType != FRAME_VISUAL) {
                                        frame = m_3DDriver->CreateFrame(frameType);
                                    } else {
                                        frame = m_3DDriver->CreateVisual(visualType);
                                    }
                                    if(frame) frame->SetName(name.c_str());
                                    created = true;
                                }
                            } break;

                            case CT_POSITION: {
                                pos = chunk.Read<S_vector>();
                                posOK = true;
                            } break;

                            case CT_WORLD_POSITION: {
                                chunk.Read<S_vector>(); // read but not used here
                            } break;

                            case CT_QUATERNION: {
                                rot = chunk.Read<S_quat>();
                                rotOK = true;
                            } break;

                            case CT_SCALE: {
                                scale = chunk.Read<S_vector>();
                                scaleOK = true;
                            } break;

                            case CT_LINK: {
                                if(++chunk == CT_NAME) {
                                    std::string name = chunk.ReadNullTerminatedString();
                                    if(name == "Backdrop sector") {
                                        parentFrame = m_BackdropSector;
                                    } else {
                                        I3D_frame* parent = m_Scene->FindFrame(name.c_str(), ENUMF_ALL);
                                        if(parent) parentFrame = parent;
                                    }
                                    --chunk;
                                }
                            } break;

                            case CT_HIDDEN: {
                                isHidden = true;
                            } break;

                            case CT_MODIFY_MODELFILENAME: {
                                if(frame && frame->GetType() == FRAME_MODEL) {
                                    I3D_model* model = (I3D_model*)frame;
                                    auto modelFileName = chunk.ReadNullTerminatedString();
                                    if(model->Open(("Models\\" + modelFileName).c_str()) == I3D_OK) {
                                        g_ModelsMap.insert({model, modelFileName});
                                    }
                                }
                            } break;

                            case CT_MODIFY_VISUAL: {
                                I3D_visual* visual = (I3D_visual*)frame;
                                I3D_VISUAL_TYPE vt = static_cast<I3D_VISUAL_TYPE>(chunk.Read<uint32_t>());
                                if(visual && frame->m_eFrameType == FRAME_VISUAL && visual->GetVisualType() != vt) {
                                    I3D_visual* newVisual = m_3DDriver->CreateVisual(vt);
                                    if(newVisual) {
                                        LS3D_RESULT res = newVisual->Duplicate(visual);
                                        if(I3D_SUCCESS(res)) {
                                            newVisual->LinkTo(visual->GetParent());
                                            while(visual->NumChildren()) {
                                                visual->GetChild(0)->LinkTo(newVisual);
                                            }
                                            newVisual->Update();
                                            visual->LinkTo(nullptr);
                                            I3D_frame* modelRoot = nullptr;
                                            for(modelRoot = newVisual; (modelRoot = modelRoot->GetParent(), modelRoot) && modelRoot->GetType() != FRAME_MODEL;)
                                                ;
                                            if(modelRoot) {
                                                I3D_model* model = (I3D_model*)modelRoot;
                                                model->DeleteFrame(visual);
                                                model->AddFrame(newVisual);
                                            }
                                            frame = newVisual;
                                        }
                                    }
                                }
                            } break;

                            case CT_MODIFY_LIT_OBJECT: {
                                I3D_visual* visual = (I3D_visual*)frame;
                                if(visual && frame->GetType() == FRAME_VISUAL && visual->GetVisualType() == VISUAL_LIT_OBJECT) {
                                    I3D_lit_object* object = (I3D_lit_object*)visual;
                                    object->Load(chunk.GetReader()->GetDescriptor());
                                }
                            } break;

                            case CT_MODIFY_LIGHT: {
                                I3D_light* light = (I3D_light*)frame;
                                while(chunk) {
                                    ChunkType lct = ++chunk;
                                    switch(lct) {
                                    case CT_LIGHT_TYPE:
                                        if(light) light->SetLightType(static_cast<I3D_LIGHTTYPE>(chunk.Read<uint32_t>()));
                                        break;
                                    case CT_LIGHT_POWER:
                                        if(light) light->SetPower(chunk.Read<float>());
                                        break;
                                    case CT_LIGHT_CONE:
                                        if(light) light->SetCone(chunk.Read<float>(), chunk.Read<float>());
                                        break;
                                    case CT_LIGHT_RANGE:
                                        if(light) light->SetRange(chunk.Read<float>(), chunk.Read<float>());
                                        break;
                                    case CT_LIGHT_MODE:
                                        if(light) light->SetMode(chunk.Read<uint32_t>());
                                        break;
                                    case CT_LIGHT_SECTOR: {
                                        auto sectorName = chunk.ReadNullTerminatedString();
                                        I3D_sector* sector = nullptr;
                                        if(sectorName == "Backdrop sector") sector = m_BackdropSector;
                                        else if(sectorName == "Primary sector") sector = m_PrimarySector;
                                        else sector = (I3D_sector*)m_Scene->FindFrame(sectorName.c_str(), ENUMF_SECTOR);
                                        if(sector && light) sector->AddLight(light);
                                    } break;
                                    case CT_COLOR:
                                        if(light) light->SetColor2(chunk.Read<S_vector>());
                                        break;
                                    default: break;
                                    }
                                    --chunk;
                                }
                            } break;

                            case CT_MODIFY_SOUND: {
                                I3D_sound* sound = (I3D_sound*)frame;
                                while(chunk) {
                                    ChunkType sct = ++chunk;
                                    switch(sct) {
                                    case CT_NAME:
                                        if(sound) {
                                            auto soundFileName = chunk.ReadNullTerminatedString();
                                            if(sound->Open(("Sounds\\" + soundFileName).c_str(), 0, nullptr, nullptr) == I3D_OK) {
                                                g_SoundsMap.insert({sound, soundFileName});
                                            }
                                        }
                                        break;
                                    case CT_SOUND_TYPE:
                                        if(sound) sound->SetSoundType(static_cast<I3D_SOUNDTYPE>(chunk.Read<uint32_t>()));
                                        break;
                                    case CT_SOUND_CONE:
                                        if(sound) sound->SetCone(chunk.Read<float>(), chunk.Read<float>());
                                        break;
                                    case CT_SOUND_VOLUME:
                                        if(sound) sound->SetVolume(chunk.Read<float>());
                                        break;
                                    case CT_SOUND_OUTVOL:
                                        if(sound) sound->SetOutVol(chunk.Read<float>());
                                        break;
                                    case CT_SOUND_RANGE: {
                                        float ir = chunk.Read<float>();
                                        float or_ = chunk.Read<float>();
                                        float inf = chunk.Read<float>();
                                        float outf = chunk.Read<float>();
                                        if(sound) sound->SetRange(ir, or_, inf, outf);
                                    } break;
                                    case CT_SOUND_LOOP:
                                        if(sound) sound->SetLoop(true);
                                        break;
                                    case CT_SOUND_SECTOR: {
                                        auto sectorName = chunk.ReadNullTerminatedString();
                                        I3D_sector* sector = nullptr;
                                        if(sectorName == "Backdrop sector") sector = m_BackdropSector;
                                        else if(sectorName == "Primary sector") sector = m_PrimarySector;
                                        else sector = (I3D_sector*)m_Scene->FindFrame(sectorName.c_str(), ENUMF_SECTOR);
                                        if(sector && sound) sector->AddSound(sound);
                                    } break;
                                    default: break;
                                    }
                                    --chunk;
                                }
                            } break;
                            }
                            --chunk;
                        }

                        if(frame) {
                            if(parentFrame) frame->LinkTo(parentFrame);
                            if(posOK) frame->SetPos(pos);
                            if(rotOK) frame->SetRot(rot);
                            if(scaleOK) frame->SetScale(scale);
                            if(isHidden) frame->SetOn(false);

                            if(created && !parentFrame) {
                                S_vector wpos = frame->GetWorldPos();
                                m_Scene->SetFrameSectorPos(frame, wpos);
                            }

                            if(frame->GetType() != FRAME_OCCLUDER) frame->Update();

                            if(created) {
                                m_Scene->AddFrame(frame);
                            }
                        }
                    }
                    --chunk;
                }
            } break;

            // CT_ACTORS and CT_SCRIPTS are editor-specific, skip them here
            default: break;
            }
            --chunk;
        }
        --chunk;
    }

    return true;
}

bool BaseEditor::LoadCacheBin(const std::string& fileName) {
    BinaryReader reader(fileName);
    if(!reader.IsOpen()) {
        // cache.bin is optional, don't log error
        return false;
    }

    ChunkReader chunk(&reader);
    if(CT_CITY == ++chunk) {
        auto version = chunk.Read<uint32_t>();
        while(chunk) {
            if(CT_PART == ++chunk) {
                auto partName = chunk.ReadString();

                I3D_frame* baseFrame = m_Scene->FindFrame(partName.c_str(), ENUMF_ALL);

                // Read part geometry data (we skip the bounding info, just need models)
                chunk.Read<S_vector2>(); // spherePos
                chunk.Read<float>();     // sphereRadius
                chunk.Read<S_vector2>(); // leftBottomCorner
                chunk.Read<S_vector2>(); // leftSide
                chunk.Read<S_vector2>(); // rightBottomCorner
                chunk.Read<S_vector2>(); // rightSide
                chunk.Read<S_vector2>(); // leftTopCorner
                chunk.Read<S_vector2>(); // bottomSide
                chunk.Read<S_vector2>(); // rightTopCorner
                chunk.Read<S_vector2>(); // topSide

                int i = 0;
                while(chunk) {
                    if(CT_FRAME == ++chunk) {
                        auto frameModel = chunk.ReadString();
                        auto pos = chunk.Read<S_vector>();
                        auto rot = chunk.Read<S_quat>();
                        auto scale = chunk.Read<S_vector>();
                        auto unk = chunk.Read<uint32_t>();
                        auto scale2 = chunk.Read<S_vector>();

                        I3D_model* model = Cache::Open(frameModel);

                        if(model) { g_ModelsMap[model] = frameModel; }

                        model->SetName((partName + "_segment" + std::to_string(i)).c_str());
                        model->LinkTo(m_PrimarySector);
                        m_Scene->AddFrame(model);

                        model->SetPos(pos);
                        model->SetRot(rot);
                        model->SetScale(scale);
                        model->SetOn(true);
                        model->Update();

                        i++;
                    }
                    --chunk;
                }
            }
            --chunk;
        }
        --chunk;
    }

    return true;
}

void BaseEditor::Update() {
    m_TargetCameraVelocity = {0, 0, 0};

    if(m_Scene) m_Scene->Tick(m_DeltaTime * 1000.0f);

    m_3DDriver->Tick(m_DeltaTime * 1000.0f);

    m_IGraph->ProcessWinMessages();
    m_IGraph->UpdateMouseData();

    S_vector rightDir = *(S_vector*)&m_Camera->m_mLocalMat.m_11;
    S_vector upDir = *(S_vector*)&m_Camera->m_mLocalMat.m_21;
    S_vector forwardDir = DirFromEuler({m_CameraRot.x, -m_CameraRot.y, 0});

    ImGui::SetCurrentContext(m_ImGuiContext);

    ImGuiIO& io = ImGui::GetIO();

    if((m_IGraph->GetMouseButtons() & 2) && (m_ViewportHovered || m_MouseEnabled)) {
        if(!m_MouseEnabled) ToggleInput(true);

        if(m_IsMovingTowardsTarget) m_IsMovingTowardsTarget = false;

        if(m_KeyboardEnabled) {
            bool isFast = ImGui::IsKeyDown(ImGuiKey_LeftShift);
            bool isSlow = ImGui::IsKeyDown(ImGuiKey_LeftControl);

            if(ImGui::IsKeyDown(ImGuiKey_W)) { m_TargetCameraVelocity += forwardDir * (isSlow ? 6.5f : (isFast ? 72.5f : 24.5f)); }
            if(ImGui::IsKeyDown(ImGuiKey_S)) { m_TargetCameraVelocity += forwardDir * -(isSlow ? 6.5f : (isFast ? 72.5f : 24.5f)); }

            if(ImGui::IsKeyDown(ImGuiKey_A)) { m_TargetCameraVelocity += rightDir * -(isSlow ? 6.5f : (isFast ? 72.5f : 24.5f)); }
            if(ImGui::IsKeyDown(ImGuiKey_D)) { m_TargetCameraVelocity += rightDir * (isSlow ? 6.5f : (isFast ? 72.5f : 24.5f)); }

            if(ImGui::IsKeyDown(ImGuiKey_Q)) { m_TargetCameraVelocity += upDir * -(isSlow ? 6.5f : (isFast ? 72.5f : 24.5f)); }
            if(ImGui::IsKeyDown(ImGuiKey_E)) { m_TargetCameraVelocity += upDir * (isSlow ? 6.5f : (isFast ? 72.5f : 24.5f)); }
        }

        auto mouseX = m_IGraph->Mouse_rx();
        auto mouseY = m_IGraph->Mouse_ry();

        m_CameraRot.x += mouseX;
        m_CameraRot.y += mouseY;

        m_CameraRot.x = NormalizeAngle(m_CameraRot.x);
        m_CameraRot.y = Clamp(m_CameraRot.y, -85, 85);
    } else {
        if(m_MouseEnabled) ToggleInput(false);
    }

    m_CurCameraVelocity = LerpVec(m_CurCameraVelocity, m_TargetCameraVelocity, 12.5f * m_DeltaTime);

    if(m_IsMovingTowardsTarget) {
        m_CameraPos = LerpVec(m_CameraPos, m_TargetPos, 16.5f * m_DeltaTime);
    } else {
        m_CameraPos += m_CurCameraVelocity * m_DeltaTime;
    }

    m_Camera->SetPos(m_CameraPos);
    m_Camera->SetDir(forwardDir, 0);

    // Set viewport dimensions
    if(m_ViewportWidth > 0 && m_ViewportHeight > 0 && m_Scene) {
        D3DVIEWPORT8 pickVP = {0, 0, m_ViewportWidth, m_ViewportHeight, 0.0f, 1.0f};
        m_D3DDevice->SetViewport(&pickVP);
        m_Scene->SetViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
        float vpAspect = (float)m_ViewportWidth / (float)m_ViewportHeight;
        m_Camera->SetAspectRatio(vpAspect);
        float hfov = CalculateHorizontalFOV(vpAspect);
        m_Camera->SetFOV(CalculateCameraFOV(hfov, vpAspect));
    }

    m_Camera->Update();

    // Subclass pre-render logic (animation ticking, raycasting, etc.)
    OnUpdate();

    // Save original RT -> redirect to viewport surface
    IDirect3DSurface8* origRT = nullptr;
    IDirect3DSurface8* origDS = nullptr;
    if(m_ViewportSurface) {
        m_D3DDevice->GetRenderTarget(&origRT);
        m_D3DDevice->GetDepthStencilSurface(&origDS);
        m_D3DDevice->SetRenderTarget(m_ViewportSurface, m_ViewportDepth);
        D3DVIEWPORT8 vp = {0, 0, m_ViewportWidth, m_ViewportHeight, 0.0f, 1.0f};
        m_D3DDevice->SetViewport(&vp);
    }

    m_IGraph->Clear(0xFF000000, 1.0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);

    if(m_SceneLoaded) {
        m_Scene->Render();
    }

    // Subclass 3D drawing
    OnRender3D();

    // Restore original RT
    if(origRT || origDS) {
        m_D3DDevice->SetRenderTarget(origRT, origDS);
        if(origRT) origRT->Release();
        if(origDS) origDS->Release();

        D3DVIEWPORT8 bbVP = {0, 0, (DWORD)m_IGraph->Scrn_sx(), (DWORD)m_IGraph->Scrn_sy(), 0.0f, 1.0f};
        m_D3DDevice->SetViewport(&bbVP);
    }

    // Clear backbuffer for ImGui
    m_D3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF1E1E1E, 1.0f, 0);

    ImGui_ImplWin32_NewFrame();
    ImGui_ImplDX8_NewFrame();
    ImGui::NewFrame();

    if(g_Editor->GetSettings()->video.fullscreen) { ImGui::RenderMainMenuBar(m_MenuBar, m_IGraph->GetMainHWND()); }

    // Dockspace
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    // Setup default layout on first run
    static bool dockLayoutInitialized = false;
    if(!dockLayoutInitialized) {
        dockLayoutInitialized = true;

        if(ImGui::DockBuilderGetNode(dockspace_id) == NULL || ImGui::DockBuilderGetNode(dockspace_id)->IsLeafNode()) {
            OnDockLayout(dockspace_id);
        }
    }

    // Viewport window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if(ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImVec2 viewSize = ImGui::GetContentRegionAvail();
        UINT newW = std::max<uint32_t>(1, (UINT)viewSize.x);
        UINT newH = std::max<uint32_t>(1, (UINT)viewSize.y);
        if(newW != m_ViewportWidth || newH != m_ViewportHeight) CreateViewportRT(newW, newH);

        m_ViewportPos = ImGui::GetCursorScreenPos();
        m_ViewportSize = viewSize;
        if(m_ViewportTexture) ImGui::Image((ImTextureID)m_ViewportTexture, viewSize);
        m_ViewportHovered = ImGui::IsItemHovered();
        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportDrawList = ImGui::GetWindowDrawList();
    }
    ImGui::End();
    ImGui::PopStyleVar();

    // Log window
    if(ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoCollapse)) {
        if(ImGui::SmallButton("Clear")) { g_LogEntries.clear(); }
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &g_LogAutoScroll);
        ImGui::Separator();

        if(ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {
            for(const auto& entry : g_LogEntries) {
                ImGui::PushStyleColor(ImGuiCol_Text, entry.color);
                ImGui::TextUnformatted(entry.message.c_str());
                ImGui::PopStyleColor();
            }
            if(g_LogAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
    ImGui::End();

    // Subclass UI
    OnRenderUI();

    // Popup messages
    if(m_DrawPopupMessage) {
        m_PopupMessageTime += m_DeltaTime;

        if(m_PopupMessageTime > m_PopupMessageDuration) {
            m_DrawPopupMessage = false;
            m_PopupMessageTime = 0;
        }

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImVec2 windowPos = ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f);
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        if(m_PopupMessageTime >= m_PopupMessageFadeOutStart) {
            float fadeAlpha = ((m_PopupMessageDuration - m_PopupMessageFadeOutStart) - (m_PopupMessageTime - m_PopupMessageFadeOutStart)) /
                              (m_PopupMessageDuration - m_PopupMessageFadeOutStart);
            ImGui::SetNextWindowBgAlpha(fadeAlpha);

            auto& style = ImGui::GetStyle();
            ImGui::PushStyleColor(
                ImGuiCol_Border,
                {style.Colors[ImGuiCol_Border].x,
                 style.Colors[ImGuiCol_Border].y,
                 style.Colors[ImGuiCol_Border].z,
                 style.Colors[ImGuiCol_Border].w * fadeAlpha});
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);

        if(ImGui::Begin("NotificationOverlay",
                        NULL,
                        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            if(m_PopupMessageTime >= m_PopupMessageFadeOutStart) {
                float fadeAlpha = ((m_PopupMessageDuration - m_PopupMessageFadeOutStart) - (m_PopupMessageTime - m_PopupMessageFadeOutStart)) /
                                  (m_PopupMessageDuration - m_PopupMessageFadeOutStart);
                ImGui::TextColored({1.0f, 1.0f, 1.0f, fadeAlpha}, "%s", m_PopupMessage.c_str());
            } else {
                ImGui::Text("%s", m_PopupMessage.c_str());
            }
            ImGui::PopFont();
            ImGui::End();
        }

        ImGui::PopStyleVar();

        if(m_PopupMessageTime >= m_PopupMessageFadeOutStart) { ImGui::PopStyleColor(); }
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX8_RenderDrawData(ImGui::GetDrawData());

    // FPS limiter
    QueryPerformanceCounter((LARGE_INTEGER*)&g_FrameEnd);
    g_ElapsedTime = ElapsedMicroseconds(g_FrameStart, g_FrameEnd);

    while(g_ElapsedTime < BASE_TARGET_FRAME_TIME) {
        if((g_ElapsedTime + g_OverSleepDuration) >= BASE_TARGET_FRAME_TIME) {
            g_OverSleepDuration -= BASE_TARGET_FRAME_TIME - g_ElapsedTime;
            break;
        }

        Sleep(1);

        QueryPerformanceCounter((LARGE_INTEGER*)&g_FrameEnd);
        g_ElapsedTime = ElapsedMicroseconds(g_FrameStart, g_FrameEnd);

        if(g_ElapsedTime > BASE_TARGET_FRAME_TIME) g_OverSleepDuration += g_ElapsedTime - BASE_TARGET_FRAME_TIME;
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&g_FrameEnd);
    g_TicksAccum += g_FrameEnd - g_FrameStart;
    g_FrameCount += 1;

    if((g_FrameCount % TARGET_FPS_CAP) == 0) {
        g_AverageFPS = ((g_QPCFreq * TARGET_FPS_CAP) + (g_TicksAccum - 1)) / g_TicksAccum;
        m_DeltaTime = 1.0f / g_AverageFPS;
        g_TicksAccum = 0;
        g_FrameCount = 0;
    }

    g_FrameStart = g_FrameEnd;

    if(!g_Editor->GetSettings()->video.fullscreen) {
        g_FramerateUpdateTime += m_DeltaTime;

        if(g_FramerateUpdateTime >= 0.35f) {
            char buf[256];
            sprintf(buf, "%s | %lld fps (%.1f ms)", m_IGraph->GetAppName(), g_AverageFPS, m_DeltaTime * 1000.0f);
            SetWindowTextA(m_IGraph->GetMainHWND(), buf);
            g_FramerateUpdateTime = 0;
        }
    }

    m_IGraph->Present();
}

void BaseEditor::Shutdown() {
    Clear();

    ReleaseViewportRT();

    ImGui::SetCurrentContext(m_ImGuiContext);

    ImGui_ImplWin32_Shutdown();
    ImGui_ImplDX8_Shutdown();

    ImGui::DestroyContext(m_ImGuiContext);

    m_SoundDriver->Close();
    m_3DDriver->Close();
    m_IGraph->Close();

    DestroyMenu(m_MenuBar);
    m_MenuBar = nullptr;

    g_ActiveEditor = nullptr;
}

void BaseEditor::ToggleInput(bool state) {
    m_MouseEnabled = state;
    m_KeyboardEnabled = state;

    if(m_MouseEnabled) {
        m_LS3DMouseDevice->SetCooperativeLevel(m_IGraph->GetMainHWND(), DISCL_EXCLUSIVE);
    } else {
        m_LS3DMouseDevice->SetCooperativeLevel(m_IGraph->GetMainHWND(), DISCL_NONEXCLUSIVE);
    }

    ShowCursor(!m_MouseEnabled);
}

void BaseEditor::Clear() {
    OnClear();

    if(m_SceneLoaded) {
        m_Scene->Close();
        m_SceneLoaded = false;

        // Release the scene entirely (required to properly clean up frames loaded from scene.4ds)
        m_Scene->Release();
        m_Scene = nullptr;
    }

    m_MissionPath = "";
    m_IGraph->SetAppName(GetEditorName());
}
