#pragma once

#include <imgui.h>
#include <vector>
#include <Windows.h>

struct MenuItem {
    std::string label;
    UINT id;
    bool isSubmenu;
    bool isEnabled;
    bool isChecked;
    bool isSeparator;
    HMENU submenu;
};

static void ExtractMenuItems(HMENU hMenu, std::vector<MenuItem>& items) {
    int count = GetMenuItemCount(hMenu);
    for(int i = 0; i < count; ++i) {
        MENUITEMINFOA mii = {sizeof(MENUITEMINFOA)};
        mii.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_FTYPE; // Use MIIM_TYPE for compatibility
        mii.dwTypeData = nullptr;

        // Get required buffer size for menu item text
        if(!GetMenuItemInfoA(hMenu, i, TRUE, &mii)) { continue; }

        std::vector<char> buffer(mii.cch + 1);
        mii.dwTypeData = buffer.data();
        mii.cch = buffer.size();

        if(!GetMenuItemInfoA(hMenu, i, TRUE, &mii)) { continue; }

        MenuItem item;
        item.label = mii.dwTypeData ? mii.dwTypeData : "";
        item.id = mii.wID;
        item.isSubmenu = (mii.hSubMenu != nullptr);
        item.isEnabled = !(mii.fState & MFS_DISABLED);
        item.isChecked = (mii.fState & MFS_CHECKED);
        item.isSeparator = (mii.fType & MFT_SEPARATOR) != 0; // Primary check for separator
        item.submenu = mii.hSubMenu;

        // Fallback check for separators: no label and no ID
        if(!item.isSeparator && item.label.empty() && item.id == 0 && !item.isSubmenu) { item.isSeparator = true; }

        items.push_back(item);
    }
}

static void BuildImGuiMenu(const std::vector<MenuItem>& items, HWND hWnd) {
    for(const auto& item: items) {
        if(item.isSubmenu && item.submenu) {
            if(ImGui::BeginMenu(item.label.c_str())) {
                std::vector<MenuItem> subItems;
                ExtractMenuItems(item.submenu, subItems);
                BuildImGuiMenu(subItems, hWnd);
                ImGui::EndMenu();
            }
        } else if(item.isSeparator) {
            ImGui::Separator();
        } else {
            if(ImGui::MenuItem(item.label.c_str(), nullptr, item.isChecked, item.isEnabled)) { PostMessage(hWnd, WM_COMMAND, item.id, 0); }
        }
    }
}

namespace ImGui {
    static void InputUInt8(const char* label, uint8_t* value, int step = 1, int step_fast = 10, ImGuiInputTextFlags flags = 0) {
        int temp = static_cast<int>(*value);

        if(ImGui::InputInt(label, &temp, step, step_fast, flags)) {
            if(temp < 0) temp = 0;
            if(temp > 255) temp = 255;
            *value = static_cast<uint8_t>(temp);
        }
    }

    static void InputUInt16(const char* label, uint16_t* value, int step = 1, int step_fast = 10, ImGuiInputTextFlags flags = 0) {
        int temp = static_cast<int>(*value);

        if(ImGui::InputInt(label, &temp, step, step_fast, flags)) {
            if(temp < 0) temp = 0;
            if(temp > UINT16_MAX) temp = UINT16_MAX;
            *value = static_cast<uint16_t>(temp);
        }
    }

    static void NonCollapsingHeader(const char* label) {
        using namespace ImGui;
        PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Header));
        PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Header));
        PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Header));
        Button(label, ImVec2(-FLT_MIN, 0.0f));
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 arrow_pos = ImVec2(GetItemRectMax().x - style.FramePadding.x - GetFontSize(), GetItemRectMin().y + style.FramePadding.y);
        PopStyleVar();
        PopStyleColor(3);
    }

    static void RenderMainMenuBar(HMENU hMenu, HWND hWnd) {
        if(ImGui::BeginMainMenuBar()) {
            std::vector<::MenuItem> items;
            ExtractMenuItems(hMenu, items);
            BuildImGuiMenu(items, hWnd);
            ImGui::EndMainMenuBar();
        }
    }

    static void RenderMenuBar(HMENU hMenu, HWND hWnd) {
        if(ImGui::BeginMenuBar()) {
            std::vector<::MenuItem> items;
            ExtractMenuItems(hMenu, items);
            BuildImGuiMenu(items, hWnd);
            ImGui::EndMenuBar();
        }
    }
} // namespace ImGui