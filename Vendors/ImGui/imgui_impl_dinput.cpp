#include "imgui_impl_dinput.h"
#include <imgui.h>
#include <stdio.h>

// Get DirectInput data for the current ImGui context
ImGui_ImplDInput_Data* ImGui_ImplDInput_GetData()
{
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx)
        return nullptr;
    return (ImGui_ImplDInput_Data*)ImGui::GetIO().UserData;
}

// Map DIK codes to ImGui keys
static void ImGui_ImplDInput_MapKeys()
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = DIK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = DIK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = DIK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = DIK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = DIK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = DIK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = DIK_NEXT;
    io.KeyMap[ImGuiKey_Home] = DIK_HOME;
    io.KeyMap[ImGuiKey_End] = DIK_END;
    io.KeyMap[ImGuiKey_Insert] = DIK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = DIK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = DIK_BACK;
    io.KeyMap[ImGuiKey_Space] = DIK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = DIK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = DIK_ESCAPE;
    io.KeyMap[ImGuiKey_KeypadEnter] = DIK_NUMPADENTER;
    io.KeyMap[ImGuiKey_A] = DIK_A;
    io.KeyMap[ImGuiKey_B] = DIK_B;
    io.KeyMap[ImGuiKey_C] = DIK_C;
    io.KeyMap[ImGuiKey_D] = DIK_D;
    io.KeyMap[ImGuiKey_E] = DIK_E;
    io.KeyMap[ImGuiKey_F] = DIK_F;
    io.KeyMap[ImGuiKey_G] = DIK_G;
    io.KeyMap[ImGuiKey_H] = DIK_H;
    io.KeyMap[ImGuiKey_I] = DIK_I;
    io.KeyMap[ImGuiKey_J] = DIK_J;
    io.KeyMap[ImGuiKey_K] = DIK_K;
    io.KeyMap[ImGuiKey_L] = DIK_L;
    io.KeyMap[ImGuiKey_M] = DIK_M;
    io.KeyMap[ImGuiKey_N] = DIK_N;
    io.KeyMap[ImGuiKey_O] = DIK_O;
    io.KeyMap[ImGuiKey_P] = DIK_P;
    io.KeyMap[ImGuiKey_Q] = DIK_Q;
    io.KeyMap[ImGuiKey_R] = DIK_R;
    io.KeyMap[ImGuiKey_S] = DIK_S;
    io.KeyMap[ImGuiKey_T] = DIK_T;
    io.KeyMap[ImGuiKey_U] = DIK_U;
    io.KeyMap[ImGuiKey_V] = DIK_V;
    io.KeyMap[ImGuiKey_W] = DIK_W;
    io.KeyMap[ImGuiKey_X] = DIK_X;
    io.KeyMap[ImGuiKey_Y] = DIK_Y;
    io.KeyMap[ImGuiKey_Z] = DIK_Z;
    io.KeyMap[ImGuiKey_0] = DIK_0;
    io.KeyMap[ImGuiKey_1] = DIK_1;
    io.KeyMap[ImGuiKey_2] = DIK_2;
    io.KeyMap[ImGuiKey_3] = DIK_3;
    io.KeyMap[ImGuiKey_4] = DIK_4;
    io.KeyMap[ImGuiKey_5] = DIK_5;
    io.KeyMap[ImGuiKey_6] = DIK_6;
    io.KeyMap[ImGuiKey_7] = DIK_7;
    io.KeyMap[ImGuiKey_8] = DIK_8;
    io.KeyMap[ImGuiKey_9] = DIK_9;
    io.KeyMap[ImGuiKey_F1] = DIK_F1;
    io.KeyMap[ImGuiKey_F2] = DIK_F2;
    io.KeyMap[ImGuiKey_F3] = DIK_F3;
    io.KeyMap[ImGuiKey_F4] = DIK_F4;
    io.KeyMap[ImGuiKey_F5] = DIK_F5;
    io.KeyMap[ImGuiKey_F6] = DIK_F6;
    io.KeyMap[ImGuiKey_F7] = DIK_F7;
    io.KeyMap[ImGuiKey_F8] = DIK_F8;
    io.KeyMap[ImGuiKey_F9] = DIK_F9;
    io.KeyMap[ImGuiKey_F10] = DIK_F10;
    io.KeyMap[ImGuiKey_F11] = DIK_F11;
    io.KeyMap[ImGuiKey_F12] = DIK_F12;
    io.KeyMap[ImGuiKey_Keypad0] = DIK_NUMPAD0;
    io.KeyMap[ImGuiKey_Keypad1] = DIK_NUMPAD1;
    io.KeyMap[ImGuiKey_Keypad2] = DIK_NUMPAD2;
    io.KeyMap[ImGuiKey_Keypad3] = DIK_NUMPAD3;
    io.KeyMap[ImGuiKey_Keypad4] = DIK_NUMPAD4;
    io.KeyMap[ImGuiKey_Keypad5] = DIK_NUMPAD5;
    io.KeyMap[ImGuiKey_Keypad6] = DIK_NUMPAD6;
    io.KeyMap[ImGuiKey_Keypad7] = DIK_NUMPAD7;
    io.KeyMap[ImGuiKey_Keypad8] = DIK_NUMPAD8;
    io.KeyMap[ImGuiKey_Keypad9] = DIK_NUMPAD9;
    io.KeyMap[ImGuiKey_KeypadMultiply] = DIK_MULTIPLY;
    io.KeyMap[ImGuiKey_KeypadAdd] = DIK_ADD;
    io.KeyMap[ImGuiKey_KeypadSubtract] = DIK_SUBTRACT;
    io.KeyMap[ImGuiKey_KeypadDecimal] = DIK_DECIMAL;
    io.KeyMap[ImGuiKey_KeypadDivide] = DIK_DIVIDE;
}

// Initialize DirectInput for the current ImGui context
bool ImGui_ImplDInput_Init(HINSTANCE hInstance, HWND hwnd)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplDInput_Data* data = new ImGui_ImplDInput_Data();
    io.UserData = data;
    data->pDI = nullptr;
    data->pKeyboard = nullptr;
    data->pMouse = nullptr;

    HRESULT hr;

    // Create DirectInput object
    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&data->pDI, nullptr);
    if (FAILED(hr))
    {
        printf("Failed to create DirectInput object\n");
        delete data;
        return false;
    }

    // Create keyboard device
    hr = data->pDI->CreateDevice(GUID_SysKeyboard, &data->pKeyboard, nullptr);
    if (FAILED(hr))
    {
        printf("Failed to create keyboard device\n");
        data->pDI->Release();
        delete data;
        return false;
    }

    hr = data->pKeyboard->SetDataFormat(&c_dfDIKeyboard);
    if (FAILED(hr))
    {
        printf("Failed to set keyboard data format\n");
        data->pKeyboard->Release();
        data->pDI->Release();
        delete data;
        return false;
    }

    hr = data->pKeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if (FAILED(hr))
    {
        printf("Failed to set keyboard cooperative level\n");
        data->pKeyboard->Release();
        data->pDI->Release();
        delete data;
        return false;
    }

    data->pKeyboard->Acquire();

    // Create mouse device
    hr = data->pDI->CreateDevice(GUID_SysMouse, &data->pMouse, nullptr);
    if (FAILED(hr))
    {
        printf("Failed to create mouse device\n");
        data->pKeyboard->Unacquire();
        data->pKeyboard->Release();
        data->pDI->Release();
        delete data;
        return false;
    }

    hr = data->pMouse->SetDataFormat(&c_dfDIMouse);
    if (FAILED(hr))
    {
        printf("Failed to set mouse data format\n");
        data->pMouse->Release();
        data->pKeyboard->Unacquire();
        data->pKeyboard->Release();
        data->pDI->Release();
        delete data;
        return false;
    }

    hr = data->pMouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if (FAILED(hr))
    {
        printf("Failed to set mouse cooperative level\n");
        data->pMouse->Release();
        data->pKeyboard->Unacquire();
        data->pKeyboard->Release();
        data->pDI->Release();
        delete data;
        return false;
    }

    data->pMouse->Acquire();

    // Map DIK codes to ImGui keys
    ImGui_ImplDInput_MapKeys();

    return true;
}

// Update ImGui with DirectInput keyboard and mouse state for the current context
void ImGui_ImplDInput_Update()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplDInput_Data* data = ImGui_ImplDInput_GetData();
    if (!data || !data->pKeyboard || !data->pMouse)
        return;

    // Keyboard input
    BYTE keyboardState[256];
    if (SUCCEEDED(data->pKeyboard->GetDeviceState(sizeof(keyboardState), keyboardState)))
    {
        for (int i = 0; i < 256; ++i)
        {
            io.KeysDown[i] = (keyboardState[i] & 0x80) != 0;
        }
    }
    else {
        data->pKeyboard->Acquire();
    }

    // Mouse input
    DIMOUSESTATE mouseState;
    if (SUCCEEDED(data->pMouse->GetDeviceState(sizeof(mouseState), &mouseState)))
    {
        io.MousePos.x += mouseState.lX;
        io.MousePos.y += mouseState.lY;
        io.MouseDown[0] = (mouseState.rgbButtons[0] & 0x80) != 0; // Left button
        io.MouseDown[1] = (mouseState.rgbButtons[1] & 0x80) != 0; // Right button
        io.MouseDown[2] = (mouseState.rgbButtons[2] & 0x80) != 0; // Middle button
        io.MouseWheel += mouseState.lZ / 120.0f; // Wheel delta
    }
    else {
        data->pMouse->Acquire();
    }

    // Update modifier key states
    io.KeyCtrl = (keyboardState[DIK_LCONTROL] & 0x80) || (keyboardState[DIK_RCONTROL] & 0x80);
    io.KeyShift = (keyboardState[DIK_LSHIFT] & 0x80) || (keyboardState[DIK_RSHIFT] & 0x80);
    io.KeyAlt = (keyboardState[DIK_LALT] & 0x80) || (keyboardState[DIK_RALT] & 0x80);
    io.KeySuper = false; // No super key in DirectInput
}

// Cleanup DirectInput resources for the current context
void ImGui_ImplDInput_Shutdown()
{
    ImGui_ImplDInput_Data* data = ImGui_ImplDInput_GetData();
    if (!data)
        return;

    if (data->pKeyboard)
    {
        data->pKeyboard->Unacquire();
        data->pKeyboard->Release();
        data->pKeyboard = nullptr;
    }
    if (data->pMouse)
    {
        data->pMouse->Unacquire();
        data->pMouse->Release();
        data->pMouse = nullptr;
    }
    if (data->pDI)
    {
        data->pDI->Release();
        data->pDI = nullptr;
    }

    ImGui::GetIO().UserData = nullptr;
    delete data;
}