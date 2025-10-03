#ifndef IMGUI_DINPUT_BACKEND_H
#define IMGUI_DINPUT_BACKEND_H

#include <windows.h>
#include <dinput.h>

// Structure to hold DirectInput data per ImGui context
struct ImGui_ImplDInput_Data
{
    LPDIRECTINPUT8 pDI;
    LPDIRECTINPUTDEVICE8 pKeyboard;
    LPDIRECTINPUTDEVICE8 pMouse;
};

extern ImGui_ImplDInput_Data* ImGui_ImplDInput_GetData();

// Initialize DirectInput and map DIK codes to ImGui keys
bool ImGui_ImplDInput_Init(HINSTANCE hInstance, HWND hwnd);

// Update ImGui with DirectInput keyboard and mouse state
void ImGui_ImplDInput_Update();

// Cleanup DirectInput resources
void ImGui_ImplDInput_Shutdown();

#endif // IMGUI_DINPUT_BACKEND_H