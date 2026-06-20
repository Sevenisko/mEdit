#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <IGraph/IGraph.h>
#include <ISND/ISND_driver.h>
#include <I3D/I3D_frame.h>
#include <I3D/I3D_driver.h>
#include <I3D/I3D_sector.h>
#include <I3D/I3D_scene.h>
#include <I3D/I3D_camera.h>
#include <imgui.h>
#include <d3d8.h>

#include <Config.h>
#include <Utils/MathUtils.h>

#include <map>

// Shared resource maps (models and sounds loaded by scene2.bin / cache.bin)
class I3D_model;
class I3D_sound;
extern std::map<I3D_model*, std::string> g_ModelsMap;
extern std::map<I3D_sound*, std::string> g_SoundsMap;

#define TARGET_FPS_CAP 250

// Base menu commands shared across all editors
enum BaseMenuCommand {
    BCMD_FILE_OPEN = 100,
    BCMD_FILE_SAVE,
    BCMD_FILE_CLOSE,
};

class BaseEditor {
public:
    virtual ~BaseEditor() = default;

    bool Init();
    void Update();
    void Shutdown();

    bool IsInit() const;

    bool IsOpened() const { return !m_MissionPath.empty(); }

    std::string GetMissionPath() const { return m_MissionPath; }

    I3D_scene* GetScene() const { return m_Scene; }
    I3D_camera* GetCamera() const { return m_Camera; }
    HWND GetWindowHandle() const { return m_IGraph->GetMainHWND(); }
    ImGuiContext* GetImGuiContext() const { return m_ImGuiContext; }

    void SetLoadProgress(float progress) { m_LoadPercentage = progress; }
    void DrawProgress(const std::string& title = "Loading, please wait...");

    void ShowPopupMessage(const std::string& msg, float duration = 5.0f, float fadeOutAt = 2.0f) {
        m_DrawPopupMessage = true;
        m_PopupMessageTime = 0;
        m_PopupMessageDuration = duration;
        m_PopupMessageFadeOutStart = fadeOutAt;
        m_PopupMessage = msg;
    }

    void ToggleInput(bool state);

    void MoveCameraToTarget(const S_vector& pos) {
        m_IsMovingTowardsTarget = true;
        auto dir = m_Camera->GetDir();
        dir *= 3.5f;
        m_TargetPos = pos - dir;
    }

    void CreateViewportRT(UINT width, UINT height);
    void ReleaseViewportRT();

    // Line drawing utilities
    struct LineVertex {
        float x, y, z;
        DWORD color;
    };

    void DrawBatchedLines(const std::vector<LineVertex>& vertices,
                          const std::vector<WORD>& indices = {},
                          const S_vector& color = {1, 1, 1},
                          uint8_t alpha = 0x00);
    void DrawWireframeBox(const I3D_bbox& bbox, const S_vector& color, uint8_t alpha);
    void DrawWireframeCone(const S_vector& pos, const S_vector& dir, float radius, float height, const S_vector& color, uint8_t alpha, int segments = 16);
    void DrawWireframeCylinder(const S_vector& basePos, float radius, float height, const S_vector& color, uint32_t alpha, int segments = 16);
    void DrawWireframeFrustum(const S_vector& pos,
                              const S_vector& dir,
                              float widthTop,
                              float heightTop,
                              float widthBottom,
                              float heightBottom,
                              float height,
                              const S_vector& color,
                              uint8_t alpha);

    // Scene loading helpers
    bool LoadScene(const std::string& scenePath);
    bool LoadScene2Bin(const std::string& fileName);
    bool LoadCacheBin(const std::string& fileName);

    // Virtual interface for subclasses
    virtual void OnUpdate() {}    // Called before rendering, after camera update
    virtual void OnRender3D() {}
    virtual void OnRenderUI() {}
    virtual void OnDockLayout(ImGuiID dockspaceId) = 0;
    virtual void OnMenuSetup() {}
    virtual void OnMenuCommand(WORD cmd) {}
    virtual void OnFileOpen() {}
    virtual void OnFileClose() {}
    virtual const char* GetEditorName() = 0;
    virtual const char* GetIconPath() = 0;
    virtual void OnSceneLoaded() {}
    virtual void OnClear() {}

    void Clear();

protected:
    IGraph* m_IGraph = nullptr;
    I3D_driver* m_3DDriver = nullptr;
    ISND_driver* m_SoundDriver = nullptr;

    I3D_scene* m_Scene = nullptr;
    I3D_camera* m_Camera = nullptr;

    I3D_sector* m_PrimarySector = nullptr;
    I3D_sector* m_BackdropSector = nullptr;

    ImGuiContext* m_ImGuiContext = nullptr;

    IDirect3DDevice8* m_D3DDevice = nullptr;
    IDirectInputDevice8A* m_LS3DMouseDevice = nullptr;

    // Viewport render-to-texture
    IDirect3DTexture8* m_ViewportTexture = nullptr;
    IDirect3DSurface8* m_ViewportSurface = nullptr;
    IDirect3DSurface8* m_ViewportDepth = nullptr;
    UINT m_ViewportWidth = 0;
    UINT m_ViewportHeight = 0;
    ImVec2 m_ViewportPos = {0, 0};
    ImVec2 m_ViewportSize = {0, 0};
    bool m_ViewportHovered = false;
    bool m_ViewportFocused = false;
    ImDrawList* m_ViewportDrawList = nullptr;

    // Menus (base)
    HMENU m_MenuBar = nullptr;
    HMENU m_FileMenu = nullptr;

    // Camera
    S_vector m_CameraPos = {0, 0, 0};
    S_vector m_CurCameraVelocity = {0, 0, 0};
    S_vector m_TargetCameraVelocity = {0, 0, 0};
    S_vector2 m_CameraRot = {0, 0};
    float m_CameraFOV = 0;

    bool m_IsMovingTowardsTarget = false;
    S_vector m_TargetPos = {0, 0, 0};

    // Timing
    std::chrono::steady_clock::time_point m_LastFrameTime;
    float m_DeltaTime = 0.0f;

    // Input
    bool m_KeyboardEnabled = false;
    bool m_MouseEnabled = false;

    // State
    bool m_SceneLoaded = false;
    std::string m_MissionPath = "";

    float m_LoadPercentage = 0.0f;

    // Popup message
    bool m_DrawPopupMessage = false;
    float m_PopupMessageTime = 0;
    float m_PopupMessageDuration = 5.0f;
    float m_PopupMessageFadeOutStart = 2.0f;
    std::string m_PopupMessage = "";
};

// Global pointer for WndProc dispatch
extern BaseEditor* g_ActiveEditor;
