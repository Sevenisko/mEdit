#include <EditorApplication.h>
#include <Utils.h>
#include <Config.h>
#include <Editors/SceneEditor.h>
#include <IO/FileSystem.h>

#include <rw_data.h>

#include <imgui.h>
#include <FA6.h>
#include <map>

void* operator new(size_t size) {
    HMODULE ls3df = GetModuleHandleA("LS3DF.dll");
    return ((void*(__cdecl*)(size_t))((int)ls3df + 0x8D510))(size);
}

void operator delete(void* p) {
    HMODULE ls3df = GetModuleHandleA("LS3DF.dll");
    ((void(__cdecl*)(void*))((int)ls3df + 0x8D404))(p);
}

void SetupImGui() {
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 2;
    style.WindowRounding = 0;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.35f, 0.35f, 0.35f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.35f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.88f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.98f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.54f, 0.54f, 0.54f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.71f, 0.71f, 0.71f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.97f, 0.15f, 0.15f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.98f, 0.26f, 0.26f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.98f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.75f, 0.10f, 0.10f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.75f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.98f, 0.26f, 0.26f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.98f, 0.26f, 0.26f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.98f, 0.26f, 0.26f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.58f, 0.18f, 0.18f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.98f, 0.26f, 0.26f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.68f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.42f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.98f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    ImFontConfig config;
    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    static const ImWchar font_ranges[] = {0x0020,
                                          0x007F, // Basic Latin
                                          0x00A0,
                                          0x00FF, // Latin-1 Supplement
                                          0x0100,
                                          0x017F, // Latin Extended-A
                                          0x0180,
                                          0x024F, // Latin Extended-B
                                          0x0400,
                                          0x04FF, // Cyrillic
                                          0x0500,
                                          0x052F, // Cyrillic Supplementary
                                          0};

    io.Fonts->AddFontFromFileTTF("mEdit\\Fonts\\play.ttf", 16.0f, &config, font_ranges);
    io.Fonts->AddFontFromFileTTF("mEdit\\Fonts\\play.ttf", 32.0f, &config, font_ranges);
    io.Fonts->AddFontFromFileTTF("mEdit\\Fonts\\fa-solid-900.ttf", 16.0f, &config, icon_ranges);
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f;
    io.Fonts->Build();
}

std::vector<LogLine> g_vLogLines;
bool g_bAutoScroll = true;

void LogInfo(const char* fmt, ...) {
    LogLine& line = g_vLogLines.emplace_back();
    line.type = line.LOG_INFO;
    va_list valist;
    va_start(valist, fmt);
    char buf[512];
    vsprintf(buf, fmt, valist);
    va_end(valist);
    line.text = std::string(buf);
}

void LogWarn(const char* fmt, ...) {
    LogLine& line = g_vLogLines.emplace_back();
    line.type = line.LOG_WARNING;
    va_list valist;
    va_start(valist, fmt);
    char buf[512];
    vsprintf(buf, fmt, valist);
    va_end(valist);
    line.text = std::string(buf);
}

void LogError(const char* fmt, ...) {
    LogLine& line = g_vLogLines.emplace_back();
    line.type = line.LOG_ERROR;
    va_list valist;
    va_start(valist, fmt);
    char buf[512];
    vsprintf(buf, fmt, valist);
    va_end(valist);
    line.text = std::string(buf);
}

bool EditorApplication::Init(HINSTANCE hInstance, const std::string& cmdLine) {
    m_Settings.Load("mEdit\\settings.json");
    if(!m_Settings.IsLoaded()) {
        debugPrintf("Config not found, saving default settings.");
        m_Settings.Save("mEdit\\settings.json");
    }

    FileSystem::Init();

    SceneEditor::Get()->Init();

    m_IsRunning = true;
    return true;
}

void EditorApplication::Update() {
    bool m_CanClose = true;

    SceneEditor::Get()->Update();

    if(!SceneEditor::Get()->IsInit()) m_IsRunning = false;
}

void EditorApplication::Shutdown() {
    m_Settings.Save("mEdit\\settings.json");

    if(SceneEditor::Get()->IsInit()) SceneEditor::Get()->Shutdown();
}