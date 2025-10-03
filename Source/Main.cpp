#include <Utils.h>
#include <Windows.h>

#include <Config.h>
#include <EditorApplication.h>

EditorApplication* g_Editor;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd) {
    debugPrintf("%s: Build from %s", PROJECT_FULL_VER, __DATE__);

    if(!TEV("mEdit", LS3D_VERSION)) { return 1; }

    g_Editor = new EditorApplication();

    if(!g_Editor->Init(hInst, lpCmdLine)) {
        MessageBox(NULL, "Failed to initialize editor!\nCheck the log for details.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    while(g_Editor->IsRunning()) {
        g_Editor->Update();
    }

    g_Editor->Shutdown();

    return 0;
}