#pragma once

#include <string>
#include <vector>
#include <algorithm>

#include <IGraph/IGraph.h>
#include <I3D/I3D_driver.h>
#include <ISND/ISND_driver.h>

#include "EditorSettings.h"

class EditorWindow;

#define TARGET_FPS_CAP 250

extern void SetupImGui();
extern void LogInfo(const char* fmt, ...);
extern void LogWarn(const char* fmt, ...);
extern void LogError(const char* fmt, ...);

class EditorApplication {
  public:
    EditorApplication() {}

    bool Init(HINSTANCE hInstance, const std::string& cmdLine);
    void Update();
    void Shutdown();
    bool IsRunning() const { return m_IsRunning; }

    EditorSettings* GetSettings() { return &m_Settings; }

  private:
    bool m_IsRunning = false;

    EditorSettings m_Settings;
};

struct LogLine {
    enum LogType { LOG_INFO, LOG_WARNING, LOG_ERROR };

    LogType type;
    std::string text;
};

extern std::vector<LogLine> g_vLogLines;

extern EditorApplication* g_Editor;