#pragma once

#include <Windows.h>

static void InstallJmpHook(DWORD address, DWORD function) {
    DWORD lpflOldProtect;
    VirtualProtect((void*)address, 5, PAGE_EXECUTE_READWRITE, &lpflOldProtect);
    *(BYTE*)(address) = 0xE9;
    *(DWORD*)(address + 1) = (unsigned long)function - (address + 5);
    VirtualProtect((void*)address, 5, lpflOldProtect, &lpflOldProtect);
}