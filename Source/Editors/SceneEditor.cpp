#include <Editors/SceneEditor.h>
#include <EditorApplication.h>
#include <IO/BinaryReader.hpp>
#include <stdexcept>
#include <cmath>
#include <atomic>
#include <thread>
#include <Utils.h>
#include <I3D/Visuals/I3D_lit_object.h>
#include <lodepng.h>
#include <Cache.h>
#include <Hacks.hpp>

#include <Game/Actors/Player.h>
#include <Game/Actors/Car.h>
#include <Game/Actors/Detector.h>
#include <Game/Actors/Door.h>
#include <Game/Actors/Railway.h>
#include <Game/Actors/TrafficManager.h>
#include <Game/Actors/PedManager.h>
#include <Game/Actors/Enemy.h>
#include <Game/Actors/Physical.h>

#include <Utils/ImGuiUtils.h>
#include <Utils/WinUtils.h>

#include <imgui_stdlib.h>
#include <imgui_impl_dx8.h>
#include <imgui_impl_dinput.h>
#include <imgui_impl_win32.h>

#include <FileDialog.hpp>

#include <MinHook.h>

#define GAME_FOV 1.22173f

std::map<I3D_sound*, std::string> g_SoundsMap;
std::map<I3D_model*, std::string> g_ModelsMap;

static WNDPROC g_OriginalWndProc = NULL;
static WNDPROC g_OriginalChildWndProc = NULL;

static bool g_ChangesMade = false;

S_vector g_AABBColor(0.57f, 0.96f, 0.55f);
S_vector g_OBBColor(0.12f, 0.92f, 0.08f);
S_vector g_XTOBBColor(0.08f, 0.59f, 0.05f);
S_vector g_SphereColor(0.78f, 0.93f, 0.25f);
S_vector g_CylinderColor(0.60f, 0.75f, 0.07f);
S_vector g_FaceColor(0.25f, 0.93f, 0.84f);

std::string OpenFileDialog(FileDialog::Mode mode, const std::string& title, const std::vector<FileDialog::Filter>& filters) {
    FileDialog dialog(mode, title, filters);
    if(dialog.Show(GetIGraph()->GetMainHWND())) {
        if(dialog.GetResult().valid) { return dialog.GetResult().path; }
    }

    return "";
}

std::string ProcessFileName(const std::string& path, const std::string& newExt) {
    // Find the last directory separator
    size_t lastSlash = path.find_last_of("\\/");
    // Extract filename (including extension)
    std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);

    // Find the last dot to locate the extension
    size_t lastDot = filename.find_last_of('.');
    if(lastDot == std::string::npos) {
        // No extension found, just append new extension
        return filename + '.' + newExt;
    }

    // Replace extension
    return filename.substr(0, lastDot + 1) + newExt;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK EditorChildWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
    case WM_DESTROY:
        // Restore original WndProc to avoid crashes
        if(g_OriginalChildWndProc) {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)g_OriginalChildWndProc);
            g_OriginalChildWndProc = NULL;
        }
        break;
    }

    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

    return CallWindowProc(g_OriginalChildWndProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK EditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    std::string path = "";
    I3D_frame* frame = nullptr;
    SceneEditor::RoadCrosspoint* cross = nullptr;
    SceneEditor::RoadWaypoint* point = nullptr;
    SceneEditor::WebNode* node = nullptr;
    GameScript* script = nullptr;

    switch(msg) {
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case MCMD_FILE_OPEN:
            //debugPrintf("Menu: Open selected\n");

            path = OpenFileDialog(FileDialog::Mode::SelectDirectory, "Select mission", {});

            if(!path.empty()) SceneEditor::Get()->Load(path);

            //SceneEditor::Get()->Load("Missions\\TUTORIAL");
            return 0;
        case MCMD_FILE_SAVE:
            if(SceneEditor::Get()->IsOpened()) { SceneEditor::Get()->Save(SceneEditor::Get()->GetMissionPath()); }

            return 0;
        case MCMD_FILE_CLOSE:
            if(SceneEditor::Get()->IsOpened() && g_ChangesMade) {
                int res =
                    MessageBoxA(hwnd, "It seems like you've edited the scene.\nWould you like to save the changes?", "mEdit", MB_ICONQUESTION | MB_YESNOCANCEL);

                switch(res) {
                case IDYES: SceneEditor::Get()->Save(SceneEditor::Get()->GetMissionPath()); break;
                case IDNO: break;
                case IDCANCEL: return 0;
                }
            }

            SceneEditor::Get()->Shutdown();
            ExitProcess(0);
            return 0;
        case MCMD_VIEW_WEBNODES: SceneEditor::Get()->ToggleWebView(); return 0;
        case MCMD_VIEW_ROADPOINTS: SceneEditor::Get()->ToggleRoadView(); return 0;
        case MCMD_VIEW_CITYPARTS: SceneEditor::Get()->ToggleCityPartsView(); return 0;
        case MCMD_VIEW_COLLISIONS: SceneEditor::Get()->ToggleCollisionView(); return 0;
        case MCMD_EDIT_COPY:
            if(frame != nullptr) { SceneEditor::Get()->CopySelection(); }

            return 0;
        case MCMD_EDIT_PASTE: SceneEditor::Get()->PasteSelection(); return 0;
        case MCMD_EDIT_DUPLICATE:
            frame = SceneEditor::Get()->GetSelectedFrame();
            if(frame != nullptr) {
                SceneEditor::Get()->CopySelection();
                SceneEditor::Get()->PasteSelection();
            }
            return 0;
        case MCMD_EDIT_DELETE: SceneEditor::Get()->DeleteSelection(); return 0;
        case MCMD_CREATE_DUMMY: SceneEditor::Get()->CreateDummy(true); return 0;
        case MCMD_CREATE_SOUND: {
            path = OpenFileDialog(FileDialog::Mode::OpenFile, "Select sound file", {{"Wave Sound File (*.wav)", "*.wav"}});

            if(!path.empty()) { SceneEditor::Get()->CreateSound(ProcessFileName(path, "wav"), true); }

            return 0;
        }
        case MCMD_CREATE_LIGHT: SceneEditor::Get()->CreateLight(true); return 0;
        case MCMD_CREATE_MODEL: {
            path = OpenFileDialog(FileDialog::Mode::OpenFile, "Select model file", {{"4DS Model File (*.4ds)", "*.4ds"}});

            if(!path.empty()) { SceneEditor::Get()->CreateModel(ProcessFileName(path, "i3d"), true); }

            return 0;
        case MCMD_CREATE_PLAYER:
            frame = SceneEditor::Get()->GetSelectedFrame();
            if(frame) {
                if(SceneEditor::Get()->GetActor(frame) == nullptr) {
                    SceneEditor::Get()->CreateActor(ACTOR_PLAYER, frame);
                } else {
                    SceneEditor::Get()->ShowPopupMessage("The frame already has an actor!");
                }
            } else {
                SceneEditor::Get()->ShowPopupMessage("Cannot create an actor without a frame!");
            }
            return 0;
        case MCMD_CREATE_HUMAN:
            frame = SceneEditor::Get()->GetSelectedFrame();
            if(frame) {
                if(SceneEditor::Get()->GetActor(frame) == nullptr) {
                    SceneEditor::Get()->CreateActor(ACTOR_ENEMY, frame);
                } else {
                    SceneEditor::Get()->ShowPopupMessage("The frame already has an actor!");
                }
            } else {
                SceneEditor::Get()->ShowPopupMessage("Cannot create an actor without a frame!");
            }
            return 0;
        case MCMD_CREATE_CAR:
            frame = SceneEditor::Get()->GetSelectedFrame();
            if(frame) {
                if(SceneEditor::Get()->GetActor(frame) == nullptr) {
                    SceneEditor::Get()->CreateActor(ACTOR_CAR, frame);
                } else {
                    SceneEditor::Get()->ShowPopupMessage("The frame already has an actor!");
                }
            } else {
                SceneEditor::Get()->ShowPopupMessage("Cannot create an actor without a frame!");
            }
            return 0;
        case MCMD_CREATE_DETECTOR:
            frame = SceneEditor::Get()->GetSelectedFrame();
            if(frame) {
                if(SceneEditor::Get()->GetActor(frame) == nullptr) {
                    SceneEditor::Get()->CreateActor(ACTOR_DETECTOR, frame);
                } else {
                    SceneEditor::Get()->ShowPopupMessage("The frame already has an actor!");
                }
            } else {
                SceneEditor::Get()->ShowPopupMessage("Cannot create an actor without a frame!");
            }
            return 0;
        case MCMD_CREATE_RAILWAY:
            frame = SceneEditor::Get()->GetSelectedFrame();
            if(frame) {
                if(SceneEditor::Get()->GetActor(frame) == nullptr) {
                    SceneEditor::Get()->CreateActor(ACTOR_RAILWAY, frame);
                } else {
                    SceneEditor::Get()->ShowPopupMessage("The frame already has an actor!");
                }
            } else {
                SceneEditor::Get()->ShowPopupMessage("Cannot create an actor without a frame!");
            }
            return 0;
        case MCMD_CREATE_PEDMAN:
            frame = SceneEditor::Get()->GetSelectedFrame();
            if(frame) {
                if(SceneEditor::Get()->GetActor(frame) == nullptr) {
                    SceneEditor::Get()->CreateActor(ACTOR_PEDESTRIANS, frame);
                } else {
                    SceneEditor::Get()->ShowPopupMessage("The frame already has an actor!");
                }
            } else {
                SceneEditor::Get()->ShowPopupMessage("Cannot create an actor without a frame!");
            }
            return 0;
        case MCMD_CREATE_TRAFFICMAN:
            frame = SceneEditor::Get()->GetSelectedFrame();
            if(frame) {
                if(SceneEditor::Get()->GetActor(frame) == nullptr) {
                    SceneEditor::Get()->CreateActor(ACTOR_TRAFFIC, frame);
                } else {
                    SceneEditor::Get()->ShowPopupMessage("The frame already has an actor!");
                }
            } else {
                SceneEditor::Get()->ShowPopupMessage("Cannot create an actor without a frame!");
            }
            return 0;
        case MCMD_CREATE_PHYSICAL:
            frame = SceneEditor::Get()->GetSelectedFrame();
            if(frame) {
                if(SceneEditor::Get()->GetActor(frame) == nullptr) {
                    SceneEditor::Get()->CreateActor(ACTOR_PHYSICAL, frame);
                } else {
                    SceneEditor::Get()->ShowPopupMessage("The frame already has an actor!");
                }
            } else {
                SceneEditor::Get()->ShowPopupMessage("Cannot create an actor without a frame!");
            }
            return 0;
        }
        case MCMD_CREATE_CROSSPOINT:
            cross = SceneEditor::Get()->CreateCrosspoint();

            SceneEditor::Get()->SelectCrosspoint(cross);

            return 0;
        case MCMD_CREATE_WAYPOINT:
            point = SceneEditor::Get()->CreateWaypoint();

            SceneEditor::Get()->SelectWaypoint(point);

            return 0;
        case MCMD_CREATE_WEBNODE:
            node = SceneEditor::Get()->CreateWebNode();

            SceneEditor::Get()->SelectWebNode(node);
            return 0;
        case MCMD_CREATE_SCRIPT:
            script = SceneEditor::Get()->CreateScript();

            SceneEditor::Get()->SelectScript(script);
            return 0;
        case MCMD_WINDOW_SCENESETTINGS:
            if(!SceneEditor::Get()->IsShowingSceneSettings()) { SceneEditor::Get()->ShowSceneSettings(); }
            return 0;
        case MCMD_WINDOW_COLLISIONSETTINGS:
            if(!SceneEditor::Get()->IsShowingCollisionSettings()) { SceneEditor::Get()->ShowCollisionSettings(); }
            return 0;
        case MCMD_WINDOW_LMG:
            if(!SceneEditor::Get()->IsShowingLightmapDialog()) { SceneEditor::Get()->ShowLightmapDialog(); }
            return 0;
        }
        break;

    case WM_CLOSE: SendMessage(hwnd, WM_COMMAND, MCMD_FILE_CLOSE, 0); return 0;
    case WM_DESTROY:
        // NOTE: Restore original WndProc to avoid crashes
        if(g_OriginalWndProc) {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
            g_OriginalWndProc = NULL;
        }
        break;
    }

    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

    return CallWindowProc(g_OriginalWndProc, hwnd, msg, wParam, lParam);
}

std::vector<I3D_frame*> IterateChildFrames(I3D_frame* frame) {
    if(!frame || !frame->m_pSzName) return {};

    std::vector<I3D_frame*> frames;

    for(uint32_t i = 0; i < frame->NumChildren(); i++) {
        frames.push_back(frame->GetChild(i));
    }

    return frames;
}

void SceneEditor::IterateFrames(I3D_frame* frame, I3D_FRAME_TYPE type) {
    if(!frame || !frame->m_pSzName) return;

    // Track if the frame was clicked for selection or double-click
    bool wasClicked = false;
    bool wasDoubleClicked = false;

    if(frame->NumChildren() > 0) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if(m_ReferencedData == frame) { flags |= ImGuiTreeNodeFlags_Selected; }

        bool isOpen = ImGui::TreeNodeEx(frame->GetName(), flags, "%s", frame->GetName());

        // Check for clicks and double-clicks
        if(ImGui::IsItemClicked(ImGuiMouseButton_Left)) { wasClicked = true; }

        if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { wasDoubleClicked = true; }

        if(isOpen) {
            for(uint32_t i = 0; i < frame->NumChildren(); i++) {
                I3D_frame* child = frame->GetChild(i);
                if(child->GetType() == type) { IterateFrames(child, type); }
            }
            ImGui::TreePop();
        }
    } else {
        bool selected = ImGui::Selectable(frame->GetName(), m_ReferencedData == frame);
        if(selected) { wasClicked = true; }

        if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { wasDoubleClicked = true; }
    }

    // Handle selection and double-click actions
    if(wasClicked && m_ReferencedData != frame) { m_ReferencedData = frame; }
    if(ImGui::IsItemHovered() && wasDoubleClicked) {
        m_HasSelectedReference = true;
        m_ShowingRefSelection = false;
    }
}

void SceneEditor::ShowFrameEntry(const HierarchyEntry& entry) {
    if(!entry.frame || !entry.frame->GetName()) return;

    // Track if the frame was clicked for selection or double-click
    bool wasClicked = false;
    bool wasDoubleClicked = false;

    // Set tree node flags
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if(entry.children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf; // No arrow for leaf nodes
    }
    if(SceneEditor::Get()->GetSelectedFrame() == entry.frame) { flags |= ImGuiTreeNodeFlags_Selected; }

    // Render tree node
    bool isOpen = ImGui::TreeNodeEx(entry.frame->GetName(), flags, "%s", entry.frame->GetName());

    // Check for clicks and double-clicks
    if(ImGui::IsItemClicked(ImGuiMouseButton_Left)) { wasClicked = true; }

    if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { wasDoubleClicked = true; }

    // Show tooltip on hover with Shift
    if(ImGui::IsItemHovered() && ImGui::GetIO().KeyShift) {
        ImGui::BeginTooltip();
        ImGui::Text("Frame type: %s", GetFrameTypeName(entry.frame->GetType()));
        ImGui::EndTooltip();
    }

    // Drag source
    if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload("FRAME", &entry.frame, sizeof(I3D_frame*));
        ImGui::Text("%s", entry.frame->GetName());
        debugPrintf("Dragging frame: %s (ptr: %p)\n", entry.frame->GetName(), entry.frame);
        ImGui::EndDragDropSource();
    }

    // Drop target
    if(ImGui::BeginDragDropTarget()) {
        if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FRAME")) {
            if(payload->DataSize != sizeof(I3D_frame*)) {
                debugPrintf("Drop rejected: Invalid payload size (%d, expected %zu)\n", payload->DataSize, sizeof(I3D_frame*));
            } else {
                I3D_frame* draggedFrame = *(I3D_frame**)payload->Data;
                if(draggedFrame && draggedFrame != entry.frame) {
                    I3D_frame* currentParent = draggedFrame->GetParent();
                    if(currentParent && currentParent->GetParent() == entry.frame) {
                        // Reparent to grandparent (entry.frame)
                        debugPrintf("Reparenting %s (ptr: %p) from parent %s to grandparent %s (ptr: %p)\n",
                                    draggedFrame->GetName(),
                                    draggedFrame,
                                    currentParent->GetName(),
                                    entry.frame->GetName(),
                                    entry.frame);
                        draggedFrame->LinkTo(entry.frame, 0);
                        UpdateHierarchyAfterDragDrop(draggedFrame, entry.frame);
                    } else {
                        // Regular reparenting to any other frame
                        debugPrintf("Dropped frame: %s (ptr: %p) onto target: %s (ptr: %p)\n",
                                    draggedFrame->GetName(),
                                    draggedFrame,
                                    entry.frame->GetName(),
                                    entry.frame);
                        draggedFrame->LinkTo(entry.frame, 0);
                        UpdateHierarchyAfterDragDrop(draggedFrame, entry.frame);
                    }
                } else {
                    debugPrintf("Drop rejected: draggedFrame=%p, same=%d\n", draggedFrame, draggedFrame == entry.frame);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Right-click context menu
    if(ImGui::BeginPopupContextItem()) {
        if(ImGui::MenuItem("Duplicate")) {
            CopyFrame(entry.frame);
            PasteFrame();
        }
        if(GetActor(entry.frame)) {
            if(ImGui::MenuItem("Remove actor")) { DeleteActor(entry.frame); }
        } else {
            if(ImGui::BeginMenu("Add actor")) {
                if(ImGui::MenuItem("Player")) { CreateActor(ACTOR_PLAYER, entry.frame); }
                if(ImGui::MenuItem("Human")) { CreateActor(ACTOR_ENEMY, entry.frame); }
                if(ImGui::MenuItem("Car")) { CreateActor(ACTOR_CAR, entry.frame); }
                if(ImGui::MenuItem("Detector")) { CreateActor(ACTOR_DETECTOR, entry.frame); }
                if(ImGui::MenuItem("Railway")) { CreateActor(ACTOR_RAILWAY, entry.frame); }
                if(ImGui::MenuItem("PED Manager")) { CreateActor(ACTOR_PEDESTRIANS, entry.frame); }
                if(ImGui::MenuItem("Traffic Manager")) { CreateActor(ACTOR_TRAFFIC, entry.frame); }
                if(ImGui::MenuItem("Physical")) { CreateActor(ACTOR_PHYSICAL, entry.frame); }
                ImGui::EndMenu();
            }
        }
        if(ImGui::MenuItem("Delete")) { DeleteFrame(entry.frame); }
        ImGui::EndPopup();
    }

    // Handle selection and double-click actions
    if(wasClicked && GetSelectedFrame() != entry.frame) { SelectFrame(entry.frame); }
    if(ImGui::IsItemHovered() && wasDoubleClicked) { MoveCameraToTarget(entry.frame->GetWorldPos()); }

    // Recursively display children
    if(isOpen) {
        for(const auto& child: entry.children) {
            ShowFrameEntry(child);
        }
        ImGui::TreePop();
    }
}

bool SceneEditor::IsDescendant(I3D_frame* frame, I3D_frame* targetFrame) {
    if(!frame || !targetFrame) return false;

    I3D_frame* current = frame->GetParent();
    while(current) {
        if(current == targetFrame) return true;
        current = current->GetParent();
    }
    return false;
}

void SceneEditor::UpdateHierarchyAfterDragDrop(I3D_frame* draggedFrame, I3D_frame* targetFrame, bool reordering, int dropIndex) {
    // Find the dragged frame's HierarchyEntry
    HierarchyEntry* draggedEntry = FindEntryByFrame(draggedFrame);
    if(!draggedEntry) {
        debugPrintf("UpdateHierarchyAfterDragDrop: Failed to find draggedEntry for frame: %s (ptr: %p)\n", draggedFrame->GetName(), draggedFrame);
        return;
    }

    // Log the dragged entry's original location
    debugPrintf("Before update: Dragged frame %s (ptr: %p), parent=%s (ptr: %p)\n",
                draggedFrame->GetName(),
                draggedFrame,
                draggedEntry->parent ? draggedEntry->parent->frame->GetName() : "root",
                draggedEntry->parent ? draggedEntry->parent->frame : nullptr);

    // Create a new entry for the dragged frame to avoid modifying the original
    HierarchyEntry newEntry = *draggedEntry;
    newEntry.children.clear(); // Prevent duplicating subtrees
    newEntry.parent = nullptr;

    // Add to target or root first
    if(targetFrame) {
        HierarchyEntry* targetEntry = FindEntryByFrame(targetFrame);
        if(!targetEntry) {
            debugPrintf("UpdateHierarchyAfterDragDrop: Failed to find targetEntry for frame: %s (ptr: %p)\n", targetFrame->GetName(), targetFrame);
            // Revert the scene graph change if target not found
            draggedFrame->LinkTo(draggedEntry->parent ? draggedEntry->parent->frame : nullptr, 0);
            return;
        }
        newEntry.parent = targetEntry;
        targetEntry->children.push_back(newEntry);
        debugPrintf("Added %s (ptr: %p) to %s (ptr: %p)'s children\n",
                    newEntry.frame->GetName(),
                    newEntry.frame,
                    targetEntry->frame->GetName(),
                    targetEntry->frame);
    } else {
        m_Hierarchy.push_back(newEntry);
        debugPrintf("Added %s (ptr: %p) to root hierarchy\n", newEntry.frame->GetName(), newEntry.frame);
    }

    // Remove dragged entry from its current parent's children or root
    if(draggedEntry->parent) {
        auto& siblings = draggedEntry->parent->children;
        auto it = std::find_if(siblings.begin(), siblings.end(), [&](const HierarchyEntry& e) { return e.frame == draggedFrame; });
        if(it != siblings.end()) {
            siblings.erase(it);
            debugPrintf("Removed %s (ptr: %p) from parent %s (ptr: %p)'s children\n",
                        draggedFrame->GetName(),
                        draggedFrame,
                        draggedEntry->parent->frame->GetName(),
                        draggedEntry->parent->frame);
        } else {
            debugPrintf("Warning: Dragged frame %s (ptr: %p) not found in parent's children\n", draggedFrame->GetName(), draggedFrame);
        }
    } else {
        auto it = std::find_if(m_Hierarchy.begin(), m_Hierarchy.end(), [&](const HierarchyEntry& e) { return e.frame == draggedFrame; });
        if(it != m_Hierarchy.end()) {
            m_Hierarchy.erase(it);
            debugPrintf("Removed %s (ptr: %p) from root hierarchy\n", draggedFrame->GetName(), draggedFrame);
        } else {
            debugPrintf("Warning: Dragged frame %s (ptr: %p) not found in root hierarchy\n", draggedFrame->GetName(), draggedFrame);
        }
    }
}

HMODULE ls3df = nullptr;

void InvalidateImGuiResources() {
    ImGui::SetCurrentContext(SceneEditor::Get()->GetImGuiContext());

    ImGui_ImplDX8_InvalidateDeviceObjects();
}

void CreateImGuiResources() {
    ImGui::SetCurrentContext(SceneEditor::Get()->GetImGuiContext());

    ImGui_ImplDX8_CreateDeviceObjects();
}

typedef int(__cdecl* dbgPrintf_t)(const char* fmt, ...);
dbgPrintf_t dbgPrintf_orig = nullptr;

int dbgPrintf_Hook(const char* fmt, ...) {
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

__declspec(naked) void ResetDevice_Before() {
    static DWORD address = (int)ls3df + 0x6D5EE;

    __asm {
		call InvalidateImGuiResources
		mov eax, address
		jmp eax
    }
}

__declspec(naked) void ResetDevice_After() {
    static DWORD address = (int)ls3df + 0x6D62C;

    *(bool*)((int)ls3df + 0xC59B3) = false; // g_SceneBegun = false

    __asm {
		call CreateImGuiResources
		mov eax, address
		jmp eax
    }
}

DWORD ToARGB(float r, float g, float b, float a) {
    uint8_t a_byte = static_cast<uint8_t>(a * 255.0f > 255.0f ? 255 : a * 255.0f);
    uint8_t r_byte = static_cast<uint8_t>(r * 255.0f > 255.0f ? 255 : r * 255.0f);
    uint8_t g_byte = static_cast<uint8_t>(g * 255.0f > 255.0f ? 255 : g * 255.0f);
    uint8_t b_byte = static_cast<uint8_t>(b * 255.0f > 255.0f ? 255 : b * 255.0f);
    return (a_byte << 24) | (r_byte << 16) | (g_byte << 8) | b_byte;
}

LS3D_RESULT DrawArrow3D(const S_vector& pos, const S_vector& direction, float length, const S_vector& color, uint8_t alpha) {
    if(direction.Magnitude2() < MRG_ZERO) return I3DERR_INVALIDPARAM;

    I3D_driver* driver = I3DGetDriver();

    // Normalize direction vector
    S_vector dir = direction;
    dir.SetNormalized(dir);

    S_vector position = pos;

    // Calculate end point of arrow shaft
    S_vector shaftEnd = position + dir * length;

    // Draw the arrow shaft
    driver->DrawLine(position, shaftEnd, color, alpha);

    // Create arrowhead using three lines
    const float headLength = length * 0.2f; // 20% of total length
    const float headRadius = length * 0.1f; // 10% of length for radius

    // Calculate base points perpendicular to direction
    S_vector up(0.0f, 1.0f, 0.0f);
    if(fabs(dir.Dot(up)) > 0.99f) // If direction is too close to up vector
        up = S_vector(1.0f, 0.0f, 0.0f);

    S_vector right = dir.Cross(up);
    right.SetNormalized(right);
    S_vector perp = dir.Cross(right);
    perp.SetNormalized(perp);

    // Base points for arrowhead lines
    S_vector baseCenter = shaftEnd - dir * headLength;
    S_vector base1 = baseCenter + right * headRadius;
    S_vector base2 = baseCenter - right * headRadius;
    S_vector base3 = baseCenter + perp * headRadius;
    S_vector base4 = baseCenter - perp * headRadius;

    // Draw four lines from tip to base points
    driver->DrawLine(shaftEnd, base1, color, alpha);
    driver->DrawLine(shaftEnd, base2, color, alpha);
    driver->DrawLine(shaftEnd, base3, color, alpha);
    driver->DrawLine(shaftEnd, base4, color, alpha);

    driver->DrawLine(base1, base3, color, alpha);
    driver->DrawLine(base3, base2, color, alpha);
    driver->DrawLine(base2, base4, color, alpha);
    driver->DrawLine(base4, base1, color, alpha);

    return I3D_OK;
}

LS3D_RESULT DrawSolidBox(
    float x, float y, float width, float height, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f, LS3D_STREAM_TYPE streamType = ST_FILLED) {
    IGraph* iGraph = GetIGraph();

    // Convert color to DWORD ARGB format
    DWORD diffuse = ToARGB(r, g, b, a);
    DWORD specular = ToARGB(0, 0, 0, 0); // No specular effect

    if(streamType == ST_FILLED) {
        // Create vertices for a quad (two triangles, 6 vertices) using S_vertex_2d
        std::vector<S_vertex_2d> vertices = {
            // Triangle 1: Top-left, Bottom-left, Bottom-right
            {x, y, 0.0f, 1.0f, diffuse, specular, 0.0f, 0.0f}, // Top-left
            {x, y + height, 0.0f, 1.0f, diffuse, specular, 0.0f, 0.0f}, // Bottom-left
            {x + width, y + height, 0.0f, 1.0f, diffuse, specular, 0.0f, 0.0f}, // Bottom-right
            // Triangle 2: Top-left, Bottom-right, Top-right
            {x, y, 0.0f, 1.0f, diffuse, specular, 0.0f, 0.0f}, // Top-left
            {x + width, y + height, 0.0f, 1.0f, diffuse, specular, 0.0f, 0.0f}, // Bottom-right
            {x + width, y, 0.0f, 1.0f, diffuse, specular, 0.0f, 0.0f} // Top-right
        };

        // Draw the quad using DrawPrimitiveList
        return iGraph->DrawPrimitiveList(PT_TRIANGLELIST, // Two triangles
                                         2, // Primitive count (2 triangles)
                                         vertices.data(), // Vertex buffer
                                         ST_FILLED // Filled rendering mode
        );
    } else { // ST_BORDER
        // Create vertices for a quad outline (using PT_LINESTRIP for simplicity)
        std::vector<S_vertex_border> vertices = {
            {x, y, 0.0f, 1.0f, diffuse, specular}, // Top-left
            {x + width, y, 0.0f, 1.0f, diffuse, specular}, // Top-right
            {x + width, y + height, 0.0f, 1.0f, diffuse, specular}, // Bottom-right
            {x, y + height, 0.0f, 1.0f, diffuse, specular}, // Bottom-left
            {x, y, 0.0f, 1.0f, diffuse, specular} // Back to top-left to close
        };

        // Draw the outline using PT_LINESTRIP
        return iGraph->DrawPrimitiveList(PT_LINESTRIP, // Line strip for outline
                                         4, // Primitive count (4 lines to form a closed loop)
                                         vertices.data(), // Vertex buffer
                                         ST_BORDER // Border rendering mode
        );
    }
}

void SceneEditor::DrawProgress(const std::string& title) {
    m_IGraph->ResetRenderProps();
    m_IGraph->Clear(0xFF000000, 1.0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    //DrawSolidBox(0, 0, m_IGraph->Scrn_sx(), m_IGraph->Scrn_sy(), 0, 0, 0);
    m_3DDriver->DrawText2D((m_IGraph->Scrn_sx() / 2) - ((title.length() / 2) * 16), (m_IGraph->Scrn_sy() / 2) - 8, title.c_str(), 0, 1.0f);
    m_IGraph->Present();
}

float CalculateHorizontalFOV(float aspectRatio) {
    // Data points: (aspect ratio, horizontal FOV in degrees)
    const float points[][2] = {
        {1.0f, 80.0f}, // 1:1
        {1.25f, 86.0f}, // 5:4
        {1.333f, 90.0f}, // 4:3
        {1.6f, 108.0f}, // 16:10
        {1.777f, 115.0f}, // 16:9
        {2.370f, 135.0f}, // 21:9
        {3.556f, 150.0f} // 32:9
    };
    const int numPoints = sizeof(points) / sizeof(points[0]);

    // If aspect ratio is below the minimum, return 80 degrees
    if(aspectRatio <= points[0][0]) { return RAD(80.0f); }

    // If aspect ratio is above the maximum, return 150 degrees
    if(aspectRatio >= points[numPoints - 1][0]) { return RAD(150.0f); }

    // Find the two points to interpolate between
    for(int i = 0; i < numPoints - 1; ++i) {
        if(aspectRatio >= points[i][0] && aspectRatio <= points[i + 1][0]) {
            float x1 = points[i][0], y1 = points[i][1];
            float x2 = points[i + 1][0], y2 = points[i + 1][1];

            // Linear interpolation: y = y1 + (y2 - y1)/(x2 - x1) * (x - x1)
            return RAD((y1 + (y2 - y1) / (x2 - x1) * (aspectRatio - x1)));
        }
    }

    // Fallback (should not reach here due to above checks)
    return RAD(90.0f);
}

float CalculateCameraFOV(float horizontalFOV, float aspectRatio) { return 2.0f * atanf(tanf(horizontalFOV / 2.0f) / aspectRatio); }

INT64 g_QPCFreq;

INT32 g_FrameCount = 0;
INT64 g_FrameStart = 0, g_FrameEnd = 0;
INT64 g_AverageFPS = 0, g_TicksAccum = 0;

INT64 g_ElapsedTime, g_OverSleepDuration = 0;
const INT64 TARGET_FRAME_TIME = (1000000 / TARGET_FPS_CAP) + 1;

INT64 __forceinline ElapsedMicroseconds(INT64 startCount, INT64 endCount) {
    INT64 elapsedMicroseconds = endCount - startCount;
    elapsedMicroseconds *= 1000000;
    elapsedMicroseconds /= g_QPCFreq;
    return elapsedMicroseconds;
}

bool SceneEditor::Init() {
    MH_CreateHook((LPVOID)&dbgPrintf, (LPVOID)&dbgPrintf_Hook, (LPVOID*)&dbgPrintf_orig);
    MH_EnableHook(MH_ALL_HOOKS);

    m_IGraph = GetIGraph();
    m_3DDriver = I3DGetDriver();
    m_SoundDriver = ISndGetDriver();

    EditorSettings* settings = g_Editor->GetSettings();

    m_MenuBar = CreateMenu();

    m_FileMenu = CreatePopupMenu();
    m_ViewMenu = CreatePopupMenu();
    m_EditMenu = CreatePopupMenu();
    m_CreateMenu = CreatePopupMenu();
    m_CreateFrameMenu = CreatePopupMenu();
    m_CreateActorMenu = CreatePopupMenu();
    m_WindowMenu = CreatePopupMenu();
    m_HelpMenu = CreatePopupMenu();

    // File menu
    AppendMenu(m_FileMenu, MF_STRING, MCMD_FILE_OPEN, "Open\tCtrl+O");
    AppendMenu(m_FileMenu, MF_STRING, MCMD_FILE_SAVE, "Save\tCtrl+S");
    EnableMenuItem(m_FileMenu, MCMD_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    AppendMenu(m_FileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(m_FileMenu, MF_STRING, MCMD_FILE_CLOSE, "Exit\tAlt+F4");

    // View menu
    AppendMenu(m_ViewMenu, MF_STRING, MCMD_VIEW_WEBNODES, "Web Nodes\tCtrl+1");
    CheckMenuItem(m_ViewMenu, MCMD_VIEW_WEBNODES, MF_BYCOMMAND | MF_UNCHECKED);
    AppendMenu(m_ViewMenu, MF_STRING, MCMD_VIEW_ROADPOINTS, "Road Points\tCtrl+2");
    CheckMenuItem(m_ViewMenu, MCMD_VIEW_ROADPOINTS, MF_BYCOMMAND | MF_UNCHECKED);
    AppendMenu(m_ViewMenu, MF_STRING, MCMD_VIEW_CITYPARTS, "City Parts\tCtrl+2");
    CheckMenuItem(m_ViewMenu, MCMD_VIEW_CITYPARTS, MF_BYCOMMAND | MF_UNCHECKED);
    AppendMenu(m_ViewMenu, MF_STRING, MCMD_VIEW_COLLISIONS, "Collisions\tCtrl+4");
    CheckMenuItem(m_ViewMenu, MCMD_VIEW_COLLISIONS, MF_BYCOMMAND | MF_UNCHECKED);

    // Edit menu
    AppendMenu(m_EditMenu, MF_STRING, MCMD_EDIT_UNDO, "Undo\tCtrl+Z");
    AppendMenu(m_EditMenu, MF_STRING, MCMD_EDIT_REDO, "Redo\tCtrl+Shift+Z");
    AppendMenu(m_EditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(m_EditMenu, MF_STRING, MCMD_EDIT_COPY, "Copy\tCtrl+C");
    AppendMenu(m_EditMenu, MF_STRING, MCMD_EDIT_PASTE, "Paste\tCtrl+V");
    AppendMenu(m_EditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(m_EditMenu, MF_STRING, MCMD_EDIT_DUPLICATE, "Duplicate\tCtrl+D");
    AppendMenu(m_EditMenu, MF_STRING, MCMD_EDIT_DELETE, "Delete\tDel");

    // Create -> Frame menu
    AppendMenu(m_CreateFrameMenu, MF_STRING, MCMD_CREATE_DUMMY, "Dummy");
    AppendMenu(m_CreateFrameMenu, MF_STRING, MCMD_CREATE_MODEL, "Model");
    AppendMenu(m_CreateFrameMenu, MF_STRING, MCMD_CREATE_LIGHT, "Light");
    AppendMenu(m_CreateFrameMenu, MF_STRING, MCMD_CREATE_SOUND, "Sound");

    // Create -> Actor menu
    AppendMenu(m_CreateActorMenu, MF_STRING, MCMD_CREATE_PLAYER, "Player");
    AppendMenu(m_CreateActorMenu, MF_STRING, MCMD_CREATE_HUMAN, "Human (Enemy)");
    AppendMenu(m_CreateActorMenu, MF_STRING, MCMD_CREATE_CAR, "Car");
    AppendMenu(m_CreateActorMenu, MF_STRING, MCMD_CREATE_DETECTOR, "Detector (Trigger)");
    AppendMenu(m_CreateActorMenu, MF_STRING, MCMD_CREATE_RAILWAY, "Railway (Tram/Subway)");
    AppendMenu(m_CreateActorMenu, MF_STRING, MCMD_CREATE_PEDMAN, "Ped Manager");
    AppendMenu(m_CreateActorMenu, MF_STRING, MCMD_CREATE_TRAFFICMAN, "Traffic Manager");
    AppendMenu(m_CreateActorMenu, MF_STRING, MCMD_CREATE_PHYSICAL, "Physical object");

    // Create -> Road menu
    AppendMenu(m_CreateRoadMenu, MF_STRING, MCMD_CREATE_WAYPOINT, "Waypoint");
    AppendMenu(m_CreateRoadMenu, MF_STRING, MCMD_CREATE_CROSSPOINT, "Crosspoint");

    // Create menu
    AppendMenu(m_CreateMenu, MF_POPUP, (UINT_PTR)m_CreateFrameMenu, "Frame");
    AppendMenu(m_CreateMenu, MF_POPUP, (UINT_PTR)m_CreateActorMenu, "Actor");
    AppendMenu(m_CreateMenu, MF_POPUP, (UINT_PTR)m_CreateRoadMenu, "Road");
    AppendMenu(m_CreateMenu, MF_STRING, MCMD_CREATE_WEBNODE, "Web Node");
    AppendMenu(m_CreateMenu, MF_STRING, MCMD_CREATE_SCRIPT, "Script");

    // Window menu
    AppendMenu(m_WindowMenu, MF_STRING, MCMD_WINDOW_SCENESETTINGS, "Scene settings");
    AppendMenu(m_WindowMenu, MF_STRING, MCMD_WINDOW_COLLISIONSETTINGS, "Collision settings");
    AppendMenu(m_WindowMenu, MF_STRING, MCMD_WINDOW_LMG, "Lightmap generator");

    // Help menu
    AppendMenu(m_HelpMenu, MF_STRING, MCMD_HELP_ABOUT, "About mEdit");

    // Main menu bar
    AppendMenu(m_MenuBar, MF_POPUP, (UINT_PTR)m_FileMenu, "File");
    AppendMenu(m_MenuBar, MF_POPUP, (UINT_PTR)m_ViewMenu, "View");
    AppendMenu(m_MenuBar, MF_POPUP, (UINT_PTR)m_EditMenu, "Edit");
    AppendMenu(m_MenuBar, MF_POPUP, (UINT_PTR)m_CreateMenu, "Create");
    AppendMenu(m_MenuBar, MF_POPUP, (UINT_PTR)m_WindowMenu, "Window");
    AppendMenu(m_MenuBar, MF_POPUP, (UINT_PTR)m_HelpMenu, "Help");

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
        g_OriginalWndProc = (WNDPROC)SetWindowLongPtr(m_IGraph->GetMainHWND(), GWLP_WNDPROC, (LONG_PTR)EditorWndProc);
        g_OriginalChildWndProc = (WNDPROC)SetWindowLongPtr(m_IGraph->GetChildHWND(), GWLP_WNDPROC, (LONG_PTR)EditorChildWndProc);

        std::vector<unsigned char> imageData;

        HICON iconBig = CreateIconFromPNG("mEdit\\Images\\Icons\\SceneEditor.png", 128, 128);
        HICON icon = CreateIconFromPNG("mEdit\\Images\\Icons\\SceneEditor.png", 32, 32);

        SendMessage(m_IGraph->GetMainHWND(), WM_SETICON, ICON_SMALL, (LPARAM)icon);
        SendMessage(m_IGraph->GetMainHWND(), WM_SETICON, ICON_BIG, (LPARAM)iconBig);

        m_IGraph->SetAppName("Scene Editor");
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

        ls3df = GetModuleHandleA("LS3DF.dll");

        if(ls3df) {
            m_D3DDevice = *(IDirect3DDevice8**)((int)ls3df + 0x1C597C);
            m_LS3DMouseDevice = *(IDirectInputDevice8A**)((int)ls3df + 0x1C595C);
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
        //m_Camera->SetNearFOV(RAD(115));
        m_Camera->SetRange(0.01f, 8000.0f);

        m_ImGuiContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_ImGuiContext);

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = "mEdit\\guisettings.ini";

        m_IGraph->CreateITexture("mEdit\\Images\\Editor\\frame_light.bmp", "mEdit\\Images\\Editor\\frame_light+.bmp", 4, &m_LightIconTexture);
        m_IGraph->CreateITexture("mEdit\\Images\\Editor\\frame_sound.bmp", "mEdit\\Images\\Editor\\frame_sound+.bmp", 4, &m_SoundIconTexture);
        m_IGraph->CreateITexture("mEdit\\Images\\Editor\\frame_camera.bmp", "mEdit\\Images\\Editor\\frame_camera+.bmp", 4, &m_CameraIconTexture);
        m_IGraph->CreateITexture("mEdit\\Images\\Editor\\actor_detector.bmp", "mEdit\\Images\\Editor\\actor_detector+.bmp", 4, &m_DetectorIconTexture);
        m_IGraph->CreateITexture("mEdit\\Images\\Editor\\actor_traffic.bmp", "mEdit\\Images\\Editor\\actor_traffic+.bmp", 4, &m_TrafficIconTexture);
        m_IGraph->CreateITexture("mEdit\\Images\\Editor\\actor_peds.bmp", "mEdit\\Images\\Editor\\actor_peds+.bmp", 4, &m_PedsIconTexture);

        m_LightIconMaterial = m_3DDriver->CreateMaterial();
        if(m_LightIconMaterial) {
            m_LightIconMaterial->SetTexture(m_LightIconTexture);
            m_LightIconMaterial->SetAmbient({1, 1, 1});
            m_LightIconMaterial->SetDiffuse({1, 1, 1});
            m_LightIconMaterial->SetEmissive({0, 0, 0});
        }

        m_SoundIconMaterial = m_3DDriver->CreateMaterial();
        if(m_SoundIconMaterial) {
            m_SoundIconMaterial->SetTexture(m_SoundIconTexture);
            m_SoundIconMaterial->SetAmbient({1, 1, 1});
            m_SoundIconMaterial->SetDiffuse({1, 1, 1});
            m_SoundIconMaterial->SetEmissive({0, 0, 0});
        }

        m_CameraIconMaterial = m_3DDriver->CreateMaterial();
        if(m_CameraIconMaterial) {
            m_CameraIconMaterial->SetTexture(m_CameraIconTexture);
            m_CameraIconMaterial->SetAmbient({1, 1, 1});
            m_CameraIconMaterial->SetDiffuse({1, 1, 1});
            m_CameraIconMaterial->SetEmissive({0, 0, 0});
        }

        m_DetectorIconMaterial = m_3DDriver->CreateMaterial();
        if(m_DetectorIconMaterial) {
            m_DetectorIconMaterial->SetTexture(m_DetectorIconTexture);
            m_DetectorIconMaterial->SetAmbient({1, 1, 1});
            m_DetectorIconMaterial->SetDiffuse({1, 1, 1});
            m_DetectorIconMaterial->SetEmissive({0, 0, 0});
        }

        m_TrafficIconMaterial = m_3DDriver->CreateMaterial();
        if(m_TrafficIconMaterial) {
            m_TrafficIconMaterial->SetTexture(m_TrafficIconTexture);
            m_TrafficIconMaterial->SetAmbient({1, 1, 1});
            m_TrafficIconMaterial->SetDiffuse({1, 1, 1});
            m_TrafficIconMaterial->SetEmissive({0, 0, 0});
        }

        m_PedsIconMaterial = m_3DDriver->CreateMaterial();
        if(m_PedsIconMaterial) {
            m_PedsIconMaterial->SetTexture(m_PedsIconTexture);
            m_PedsIconMaterial->SetAmbient({1, 1, 1});
            m_PedsIconMaterial->SetDiffuse({1, 1, 1});
            m_PedsIconMaterial->SetEmissive({0, 0, 0});
        }

        QueryPerformanceFrequency((LARGE_INTEGER*)&g_QPCFreq);

        QueryPerformanceCounter((LARGE_INTEGER*)&g_FrameStart);

        SetupImGui();

        InstallJmpHook((int)ls3df + 0x6D5E4, (DWORD)&ResetDevice_Before); // NOTE: Required to release ImGui resources before resetting device
        InstallJmpHook((int)ls3df + 0x6D603, (DWORD)&ResetDevice_After); // NOTE: Required to re-init ImGui resources after resetting device

        InstallJmpHook((int)ls3df + 0x57DD4, (int)ls3df + 0x57E74); // NOTE: Always apply SetRange - no difference check

        InstallJmpHook((int)ls3df + 0x5FB65,
                       (int)ls3df + 0x5FBA1); // NOTE: Don't delete bitmaps while loading lightmaps (might leak data, but I FUCKING NEED IT)

        ImGui_ImplWin32_Init(m_IGraph->GetMainHWND());
        ImGui_ImplDX8_Init(m_D3DDevice);

        return true;
    }

    return false;
}

bool SceneEditor::IsInit() const { return m_IGraph->IsInit(); }

#define LINE_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)

void SceneEditor::DrawBatchedLines(const std::vector<LineVertex>& vertices, const std::vector<WORD>& indices, const S_vector& color, uint8_t alpha) {
    if(vertices.empty()) return;

    bool useIndexing = !indices.empty();
    if(useIndexing && indices.size() % 2 != 0) return; // Indices must be even for lines

    float calcAplha = (double)(255 - (unsigned int)alpha) * 0.0039215689;
    float a = calcAplha * 255.0;
    float r = color.x * 255.0;
    float g = color.y * 255.0;
    float b = color.z * 255.0;
    uint32_t d3dColor = (int)b | (((int)g | (((int)r | ((int)a << 8)) << 8)) << 8);

    // Apply default color if needed (or per-vertex colors could override)
    std::vector<LineVertex> coloredVerts = vertices;
    for(auto& v: coloredVerts) {
        if(v.color == 0) v.color = d3dColor;
    }

    // Create vertex buffer
    IDirect3DVertexBuffer8* pVB = nullptr;
    UINT vbSize = static_cast<UINT>(coloredVerts.size() * sizeof(LineVertex));
    m_D3DDevice->CreateVertexBuffer(vbSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, LINE_FVF, D3DPOOL_DEFAULT, &pVB);
    if(!pVB) return;

    // Fill VB
    void* pVBData = nullptr;
    pVB->Lock(0, 0, (BYTE**)&pVBData, D3DLOCK_DISCARD);
    memcpy(pVBData, coloredVerts.data(), vbSize);
    pVB->Unlock();

    // Optional index buffer
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

    // Set up rendering
    m_D3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
    m_D3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
    m_D3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_D3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    m_D3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_D3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_D3DDevice->SetRenderState(D3DRS_ALPHAREF, TRUE);
    m_D3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_D3DDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    //m_D3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    m_D3DDevice->SetTexture(0, nullptr);
    m_D3DDevice->SetStreamSource(0, pVB, sizeof(LineVertex));
    m_D3DDevice->SetVertexShader(LINE_FVF);

    // Draw
    UINT numPrimitives = useIndexing ? static_cast<UINT>(indices.size() / 2) : static_cast<UINT>(coloredVerts.size() / 2);
    if(useIndexing && pIB) {
        m_D3DDevice->DrawIndexedPrimitive(D3DPT_LINELIST, 0, static_cast<UINT>(coloredVerts.size()), 0, numPrimitives);
    } else {
        m_D3DDevice->DrawPrimitive(D3DPT_LINELIST, 0, numPrimitives);
    }

    // Cleanup
    pVB->Release();
    if(pIB) pIB->Release();
}

void SceneEditor::DrawWireframeBox(const I3D_bbox& bbox, const S_vector& color, uint8_t alpha) {
    // Define the 8 vertices of the box
    S_vector pos[8] = {
        {bbox.min.x, bbox.min.y, bbox.min.z}, // 0: min min min
        {bbox.max.x, bbox.min.y, bbox.min.z}, // 1: max min min
        {bbox.max.x, bbox.min.y, bbox.max.z}, // 2: max min max
        {bbox.min.x, bbox.min.y, bbox.max.z}, // 3: min min max
        {bbox.min.x, bbox.max.y, bbox.min.z}, // 4: min max min
        {bbox.max.x, bbox.max.y, bbox.min.z}, // 5: max max min
        {bbox.max.x, bbox.max.y, bbox.max.z}, // 6: max max max
        {bbox.min.x, bbox.max.y, bbox.max.z} // 7: min max max
    };

    std::vector<LineVertex> vertices(8);
    for(int i = 0; i < 8; ++i) {
        vertices[i] = {pos[i].x, pos[i].y, pos[i].z, 0};
    }

    // Indices for the 12 lines
    std::vector<WORD> indices = {
        0, 1, 1, 2, 2, 3, 3, 0, // Bottom face
        4, 5, 5, 6, 6, 7, 7, 4, // Top face
        0, 4, 1, 5, 2, 6, 3, 7 // Vertical edges
    };

    DrawBatchedLines(vertices, indices, color, alpha);
}

void SceneEditor::DrawWireframeCone(const S_vector& pos, const S_vector& dir, float radius, float height, const S_vector& color, uint8_t alpha, int segments) {
    // Generate vertices: one for apex, 'segments' for the base
    std::vector<S_vector> posVerts(segments + 1);

    S_vector position = pos;

    // Normalize direction vector
    S_vector nDir;
    nDir.SetNormalized(dir);

    // Apex at position
    posVerts[0] = position;

    // Base center at position + height * nDir
    S_vector baseCenter = position + nDir * height;

    // Find two perpendicular vectors to form the base plane
    S_vector up = (fabs(nDir.y) < 0.9f) ? S_vector(0.0f, 1.0f, 0.0f) : S_vector(1.0f, 0.0f, 0.0f); // Avoid near-parallel case
    S_vector u;
    u.SetNormalized(nDir.Cross(up)); // First basis vector
    S_vector v;
    v.SetNormalized(nDir.Cross(u)); // Second basis vector, perpendicular to nDir and u

    // Base vertices in the plane perpendicular to nDir at position
    for(int i = 0; i < segments; ++i) {
        float theta = 2.0f * PI * static_cast<float>(i) / segments;
        float cosTheta = cosf(theta);
        float sinTheta = sinf(theta);
        // Base vertex = baseCenter + radius * (cos(θ) * u + sin(θ) * v)
        posVerts[i + 1] = baseCenter + (u * (radius * cosTheta) + v * (radius * sinTheta));
    }

    std::vector<LineVertex> vertices(segments + 1);
    for(int i = 0; i <= segments; ++i) {
        vertices[i] = {posVerts[i].x, posVerts[i].y, posVerts[i].z, 0};
    }

    // Indices for base circle and lines to apex
    std::vector<WORD> indices;
    indices.reserve(segments * 4); // segments for base + segments for sides, each 2 indices

    // Base circle
    for(int i = 0; i < segments; ++i) {
        indices.push_back(static_cast<WORD>(1 + i));
        indices.push_back(static_cast<WORD>(1 + (i + 1) % segments));
    }

    // Lines from base to apex
    for(int i = 0; i < segments; ++i) {
        indices.push_back(static_cast<WORD>(1 + i));
        indices.push_back(0);
    }

    DrawBatchedLines(vertices, indices, color, alpha);
}

void SceneEditor::DrawWireframeCylinder(const S_vector& basePos, float radius, float height, const S_vector& color, uint32_t alpha, int segments) {
    if(m_3DDriver == nullptr || segments < 3 || radius <= 0.0f || height <= 0.0f) {
        return; // Basic validation
    }

    std::vector<S_vector> posVerts(2 * segments);

    // Generate bottom circle points
    for(int i = 0; i < segments; ++i) {
        float theta = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        float cx = radius * cosf(theta);
        float cy = radius * sinf(theta);
        posVerts[i] = {basePos.x + cx, basePos.y, basePos.z + cy};
    }

    // Generate top circle points
    for(int i = 0; i < segments; ++i) {
        float theta = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        float cx = radius * cosf(theta);
        float cy = radius * sinf(theta);
        posVerts[segments + i] = {basePos.x + cx, basePos.y + height, basePos.z + cy};
    }

    std::vector<LineVertex> vertices(2 * segments);
    for(int i = 0; i < 2 * segments; ++i) {
        vertices[i] = {posVerts[i].x, posVerts[i].y, posVerts[i].z, 0};
    }

    // Indices for bottom circle, top circle, and verticals
    std::vector<WORD> indices;
    indices.reserve(6 * segments); // 3 loops * 2 indices per line

    // Bottom circle
    for(int i = 0; i < segments; ++i) {
        indices.push_back(static_cast<WORD>(i));
        indices.push_back(static_cast<WORD>((i + 1) % segments));
    }

    // Top circle
    for(int i = 0; i < segments; ++i) {
        indices.push_back(static_cast<WORD>(segments + i));
        indices.push_back(static_cast<WORD>(segments + (i + 1) % segments));
    }

    // Vertical lines
    for(int i = 0; i < segments; ++i) {
        indices.push_back(static_cast<WORD>(i));
        indices.push_back(static_cast<WORD>(segments + i));
    }

    DrawBatchedLines(vertices, indices, color, static_cast<uint8_t>(alpha));
}

void SceneEditor::DrawWireframeFrustum(const S_vector& pos,
                                       const S_vector& dir,
                                       float widthTop,
                                       float heightTop,
                                       float widthBottom,
                                       float heightBottom,
                                       float height,
                                       const S_vector& color,
                                       uint8_t alpha) {
    // Generate vertices: 4 for top base, 4 for bottom base
    std::vector<S_vector> posVerts(8);

    S_vector position = pos;

    // Normalize direction vector
    S_vector nDir;
    nDir.SetNormalized(dir);

    // Top base center at position
    S_vector topCenter = position;
    // Bottom base center at position + height * nDir
    S_vector bottomCenter = position + nDir * height;

    // Find two perpendicular vectors to form the base planes
    S_vector up = (fabs(nDir.y) < 0.9f) ? S_vector(0.0f, 1.0f, 0.0f) : S_vector(1.0f, 0.0f, 0.0f); // Avoid near-parallel case
    S_vector u;
    u.SetNormalized(nDir.Cross(up)); // First basis vector (width direction)
    S_vector v;
    v.SetNormalized(nDir.Cross(u)); // Second basis vector (height direction)

    // Generate vertices for top base (rectangle, indices 0 to 3)
    float halfWidthTop = widthTop * 0.5f;
    float halfHeightTop = heightTop * 0.5f;
    posVerts[0] = topCenter + (u * halfWidthTop) + (v * halfHeightTop); // Top-right
    posVerts[1] = topCenter + (u * halfWidthTop) - (v * halfHeightTop); // Bottom-right
    posVerts[2] = topCenter - (u * halfWidthTop) - (v * halfHeightTop); // Bottom-left
    posVerts[3] = topCenter - (u * halfWidthTop) + (v * halfHeightTop); // Top-left

    // Generate vertices for bottom base (rectangle, indices 4 to 7)
    float halfWidthBottom = widthBottom * 0.5f;
    float halfHeightBottom = heightBottom * 0.5f;
    posVerts[4] = bottomCenter + (u * halfWidthBottom) + (v * halfHeightBottom); // Top-right
    posVerts[5] = bottomCenter + (u * halfWidthBottom) - (v * halfHeightBottom); // Bottom-right
    posVerts[6] = bottomCenter - (u * halfWidthBottom) - (v * halfHeightBottom); // Bottom-left
    posVerts[7] = bottomCenter - (u * halfWidthBottom) + (v * halfHeightBottom); // Top-left

    std::vector<LineVertex> vertices(8);
    for(int i = 0; i < 8; ++i) {
        vertices[i] = {posVerts[i].x, posVerts[i].y, posVerts[i].z, 0};
    }

    // Indices for top base, bottom base, and sides
    std::vector<WORD> indices = {
        0, 1, 1, 2, 2, 3, 3, 0, // Top base
        4, 5, 5, 6, 6, 7, 7, 4, // Bottom base
        0, 4, 1, 5, 2, 6, 3, 7 // Side edges
    };

    DrawBatchedLines(vertices, indices, color, alpha);
}

float g_FramerateUpdateTime = 0;

struct PickHit {
    I3D_frame* frame = nullptr;
    float dist = FLT_MAX;
};

struct BVHit {
    I3D_frame* frame = nullptr;
    float estDist;
    bool operator<(const BVHit& rhs) const { return estDist < rhs.estDist; }
};

PickHit g_BestHit;
S_vector g_RayOrigin{0, 0, 0}, g_RayDir{0, 0, 0};
std::vector<BVHit> g_HitCandidates;
#define MAX_PICK 50000.0f

I3DENUMRET __stdcall EnumSceneFrames(I3D_frame* frame, uint32_t user) {
    SceneEditor* editor = (SceneEditor*)user;

    if(frame->GetType() != FRAME_VISUAL) { return I3DENUMRET_OK; }
    if(!editor->GetScene()->TestVisibility(frame, editor->GetCamera())) { return I3DENUMRET_OK; }

    I3D_visual* visual = (I3D_visual*)frame;
    if(visual->GetVisualType() != VISUAL_OBJECT && visual->GetVisualType() != VISUAL_LIT_OBJECT && visual->GetVisualType() != VISUAL_SINGLE_MESH &&
       visual->GetVisualType() != VISUAL_SINGLE_MORPH && visual->GetVisualType() != VISUAL_MORPH && visual->GetVisualType() != VISUAL_BILLBOARD &&
       visual->GetVisualType() != VISUAL_MIRROR) {
        return I3DENUMRET_OK;
    }

    I3D_object* obj = (I3D_object*)visual;
    I3D_mesh_object* mesh = obj->GetMesh();

    if(!mesh) { return I3DENUMRET_OK; }

    const auto& worldBVol = frame->m_sWorldBVol;
    float tHit;
    if((tHit = RayAABB_HitDist(g_RayOrigin, g_RayDir, worldBVol.bbox.min, worldBVol.bbox.max, MAX_PICK)) < 0) { return I3DENUMRET_OK; }

    if(tHit >= 0) {
        g_HitCandidates.push_back({frame, tHit});
        return I3DENUMRET_OK;
    }

    return I3DENUMRET_OK;
}

void SceneEditor::Update() {
    m_TargetCameraVelocity = {0, 0, 0};

    if(m_Scene) m_Scene->Tick(m_DeltaTime * 1000.0f);

    for(auto model: m_AnimatedModels) {
        int animTime = model->GetAnimTime(0);
        int endTime = model->GetAnimationSet(0)->GetEndTime();

        if(animTime >= endTime) {
            model->SetAnimTime(0, 0);
            model->StartAnimation(0);
        }

        model->Tick(m_DeltaTime * 1000.0f);
        model->Update();
    }

    m_3DDriver->Tick(m_DeltaTime * 1000.0f);

    m_IGraph->ProcessWinMessages();
    m_IGraph->UpdateMouseData();

    S_vector rightDir = *(S_vector*)&m_Camera->m_mLocalMat.m_11;
    S_vector upDir = *(S_vector*)&m_Camera->m_mLocalMat.m_21;
    S_vector forwardDir = DirFromEuler({m_CameraRot.x, -m_CameraRot.y, 0});

    ImGui::SetCurrentContext(m_ImGuiContext);

    ImGuiIO& io = ImGui::GetIO();

    if(m_IGraph->GetMouseButtons() & 2) {
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
    m_Camera->Update();

    if(io.MouseClicked[0] && !ImGui::IsAnyItemHovered() && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !ImGuizmo::IsOver()) {
        LS3D_RESULT res = m_Scene->UnmapScreenPoint(io.MouseClickedPos[0].x, io.MouseClickedPos[0].y, g_RayOrigin, g_RayDir);

        if(res == I3D_OK) {
            g_BestHit.dist = FLT_MAX;
            g_BestHit.frame = nullptr;

            g_HitCandidates.clear();

            m_Scene->EnumFrames(EnumSceneFrames, (uint32_t)this, ENUMF_ALL);

            if(!g_HitCandidates.empty()) {
                std::sort(g_HitCandidates.begin(), g_HitCandidates.end());

                for(size_t i = 0; i < g_HitCandidates.size() && g_HitCandidates[i].estDist < g_BestHit.dist; ++i) {
                    float precise = TestMeshRaycast(g_RayOrigin, g_RayDir, (I3D_object*)g_HitCandidates[i].frame, g_BestHit.dist);
                    if(precise >= 0) {
                        g_BestHit.dist = precise;
                        g_BestHit.frame = g_HitCandidates[i].frame;
                    }
                }
            }

            if(g_BestHit.frame) {
                I3D_frame* ancestor = FindModelAncestor(g_BestHit.frame);

                if(ancestor != m_PrimarySector) {
                    SelectFrame(ancestor);
                } else {
                    SelectFrame(g_BestHit.frame);
                }
            } else {
                ClearSelection();
            }
        }
    }

    m_IGraph->Clear(0xFF000000, 1.0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    if(m_SceneLoaded) {
        m_Scene->Render();

        m_IGraph->SetState(ZENABLE, 1);

        if(m_ShowTransformGrid) {
            S_vector origin = {0, 0, 0};
            S_vector camPos = m_CameraPos;
            S_vector targetPos = {0, 0, 0};

            bool isSelected = true;

            switch(m_SelectionType) {
            case SEL_FRAME: targetPos = m_SelectedFrame->GetWorldPos(); break;
            case SEL_CROSSPOINT: targetPos = m_SelectedCrosspoint->pos; break;
            case SEL_WAYPOINT: targetPos = m_SelectedWaypoint->pos; break;
            case SEL_NODE: targetPos = m_SelectedNode->pos; break;
            default: isSelected = false;
            }

            const float totalGridSize = m_GridSize * m_GridScale;
            const S_vector color = {0.7f, 0.7f, 0.7f};

            S_vector snapped = SnapToGrid(camPos);

            origin = {snapped.x, 0, snapped.z};

            if(isSelected) { origin.y = targetPos.y; }

            S_matrix mat;
            mat.Identity();

            m_IGraph->SetWorldMatrix(mat);

            // Draw lines parallel to X-axis (varying Z)
            for(int i = 0; i <= m_GridSize; ++i) {
                float z = -totalGridSize / 2.0f + i * m_GridScale;
                S_vector p1 = {-totalGridSize / 2.0f, 0.0f, z};
                S_vector p2 = {totalGridSize / 2.0f, 0.0f, z};
                m_3DDriver->DrawLine(origin + p1, origin + p2, color, 0);
            }

            // Draw lines parallel to Z-axis (varying X)
            for(int i = 0; i <= m_GridSize; ++i) {
                float x = -totalGridSize / 2.0f + i * m_GridScale;
                S_vector p1 = {x, 0.0f, -totalGridSize / 2.0f};
                S_vector p2 = {x, 0.0f, totalGridSize / 2.0f};
                m_3DDriver->DrawLine(origin + p1, origin + p2, color, 0);
            }
        }

        if(m_DrawCityParts) {
            for(CityPart& part: m_CacheParts) {
                float dist = (m_CameraPos - part.frame->GetWorldPos()).Magnitude();
                if(dist <= part.sphereRadius + 256) {
                    S_matrix mat;
                    mat.Identity();
                    m_IGraph->SetWorldMatrix(mat);
                    S_vector pos = (part.bbox.min + part.bbox.max) * 0.5f;
                    m_3DDriver->DrawTextA(pos, part.name.c_str(), 0x00, 0.1f);
                }
            }
        }

        if(m_DrawWebNodes) {
            for(auto& pair: m_WebNodes) {
                WebNode& node = pair.second;
                float dist = (m_CameraPos - node.pos).Magnitude();

                if(dist <= 128) {
                    S_matrix mat;
                    mat.Identity();

                    I3D_bsphere sphere;
                    sphere.pos = node.pos;
                    sphere.radius = 0.25f;

                    char name[64];
                    sprintf(name, "Node %u", pair.first);
                    m_3DDriver->DrawTextA(node.pos, name, 0, 0.01f);
                    m_3DDriver->DrawSphere(mat, sphere, GetWebNodeTypeColor(node.type), 0);

                    for(uint8_t i = 0; i < node.numEnterLinks; i++) {
                        WebConnection& conn = node.links[i];

                        if(conn.endPoint != 0xFFFF) {
                            WebNode& targetNode = m_WebNodes[conn.endPoint];

                            m_IGraph->SetWorldMatrix(mat);
                            m_3DDriver->DrawLine(node.pos, targetNode.pos, GetWebConnTypeColor(conn.type), 0);
                        }
                    }
                }
            }
        }

        if(m_DrawRoadPoints) {
            for(auto& pair: m_RoadWaypoints) {
                RoadWaypoint& point = pair.second;

                float dist = (m_CameraPos - point.pos).Magnitude();

                if(dist <= 256) {
                    S_matrix mat;
                    mat.Identity();

                    I3D_bsphere sphere;
                    sphere.pos = point.pos;
                    sphere.radius = 0.45f;

                    char name[64];
                    sprintf(name, "Waypoint %u", pair.first);
                    m_3DDriver->DrawTextA(point.pos, name, 0, 0.01f);
                    m_3DDriver->DrawSphere(mat, sphere, {0, 1, 0}, 0);

                    if(point.nextWaypoint != UINT16_MAX) {
                        bool isWaypoint = (point.nextWaypoint & 0x8000) != 0;
                        if(isWaypoint) {
                            int id = point.nextWaypoint & 0x7FFF;

                            RoadWaypoint& targetPoint = m_RoadWaypoints[id];

                            m_IGraph->SetWorldMatrix(mat);
                            m_3DDriver->DrawLine(point.pos, targetPoint.pos, {0.85f, 0.85f, 0.85f}, 0);
                        } else {
                            RoadCrosspoint& targetCross = m_RoadCrosspoints[point.nextWaypoint];

                            m_IGraph->SetWorldMatrix(mat);
                            m_3DDriver->DrawLine(point.pos, targetCross.pos, {0.85f, 0.85f, 0.85f}, 0);
                        }
                    }

                    if(point.prevWaypoint != UINT16_MAX) {
                        bool isWaypoint = (point.prevWaypoint & 0x8000) != 0;
                        if(isWaypoint) {
                            int id = point.prevWaypoint & 0x7FFF;

                            RoadWaypoint& targetPoint = m_RoadWaypoints[id];

                            m_IGraph->SetWorldMatrix(mat);
                            m_3DDriver->DrawLine(point.pos, targetPoint.pos, {0.85f, 0.85f, 0.85f}, 0);
                        } else {
                            RoadCrosspoint& targetCross = m_RoadCrosspoints[point.prevWaypoint];

                            m_IGraph->SetWorldMatrix(mat);
                            m_3DDriver->DrawLine(point.pos, targetCross.pos, {0.85f, 0.85f, 0.85f}, 0);
                        }
                    }
                }
            }

            for(auto& pair: m_RoadCrosspoints) {
                RoadCrosspoint& cross = pair.second;
                float dist = (m_CameraPos - cross.pos).Magnitude();

                if(dist <= 256) {
                    S_matrix mat;
                    mat.Identity();

                    I3D_bsphere sphere;
                    sphere.pos = cross.pos;
                    sphere.radius = 0.45f;

                    char name[64];
                    sprintf(name, "Crosspoint %u", pair.first);
                    m_3DDriver->DrawTextA(cross.pos, name, 0, 0.01f);
                    m_3DDriver->DrawSphere(mat, sphere, {0, 0, 1}, 0);

                    for(int i = 0; i < 4; i++) {
                        if(cross.waypointLinks[i] != 0xFFFF && cross.directionLinks[i].crosspointLink != 0xFFFF) {
                            RoadWaypoint& point = m_RoadWaypoints[cross.waypointLinks[i] & 0x7FFF];
                            RoadWaypoint* nextPoint = nullptr;

                            if(point.nextWaypoint & 0x8000) {
                                nextPoint = &m_RoadWaypoints[point.nextWaypoint & 0x7FFF];
                            } else {
                                nextPoint = &m_RoadWaypoints[point.prevWaypoint & 0x7FFF];
                            }

                            S_vector frontPointDir = CalculateDirection(point.pos, nextPoint->pos);
                            S_vector rightPointDir = CalculateRightDirection(frontPointDir);

                            m_IGraph->SetWorldMatrix(mat);

                            for(int j = 0; j < 4; j++) {
                                RoadLane& lane = cross.directionLinks[i].lanes[j];

                                m_IGraph->SetWorldMatrix(mat);

                                switch(lane.type) {
                                case 1: DrawArrow3D(point.pos + (rightPointDir * (lane.distance)), frontPointDir, 3.5f, {0, 1, 0}, 0); break;
                                case 2: DrawArrow3D(point.pos + (rightPointDir * (lane.distance)), frontPointDir, 3.5f, {1, 0, 0}, 0); break;
                                case 3: DrawArrow3D(point.pos + (rightPointDir * (lane.distance)), frontPointDir, 3.5f, {1, 1, 0}, 0); break;
                                }

                                if(lane.type != 0) {}
                            }
                        }
                    }
                }
            }
        }

        for(auto frame: m_Frames) {
            bool hasDrawnFrameSprite = false;

            if(frame != m_SelectedFrame) {
                S_vector rightDir = *(S_vector*)&frame->m_mLocalMat.m_11;
                S_vector upDir = *(S_vector*)&frame->m_mLocalMat.m_21;
                S_vector forwardDir = *(S_vector*)&frame->m_mLocalMat.m_31;
                S_vector pos = frame->GetPos();

                switch(frame->GetType()) {
                case FRAME_DUMMY: {
                    m_IGraph->SetWorldMatrix(frame->m_mWorldMat);
                    m_3DDriver->DrawLine(pos, pos + (forwardDir * 1.5f), {1, 0, 0}, 0);
                    m_3DDriver->DrawLine(pos, pos + (upDir * 1.5f), {0, 1, 0}, 0);
                    m_3DDriver->DrawLine(pos, pos + (rightDir * 1.5f), {0, 0, 1}, 0);
                    hasDrawnFrameSprite = true;
                } break;
                case FRAME_SOUND: {
                    m_IGraph->SetWorldMatrix(frame->m_mWorldMat);
                    m_3DDriver->DrawSprite(pos, m_SoundIconMaterial, 0, 1.0f);
                    hasDrawnFrameSprite = true;
                } break;
                case FRAME_LIGHT: {
                    m_IGraph->SetWorldMatrix(frame->m_mWorldMat);
                    I3D_light* light = (I3D_light*)frame;
                    m_3DDriver->DrawSprite(pos, m_LightIconMaterial, 0, 1.0f);
                    hasDrawnFrameSprite = true;
                } break;
                case FRAME_CAMERA: {
                    m_IGraph->SetWorldMatrix(frame->m_mWorldMat);
                    I3D_camera* light = (I3D_camera*)frame;
                    m_3DDriver->DrawSprite(pos, m_CameraIconMaterial, 0, 1.0f);
                    hasDrawnFrameSprite = true;
                } break;
                }
            }

            Actor* actor = GetActor(frame);

            if(actor) {
                S_vector pos = frame->GetPos();

                if(hasDrawnFrameSprite) { pos.y += 1.5f; }

                switch(actor->GetType()) {
                case ACTOR_DETECTOR:
                    m_IGraph->SetWorldMatrix(frame->m_mWorldMat);
                    m_3DDriver->DrawSprite(pos, m_DetectorIconMaterial, 0, 1.0f);
                    break;
                case ACTOR_TRAFFIC:
                    m_IGraph->SetWorldMatrix(frame->m_mWorldMat);
                    m_3DDriver->DrawSprite(pos, m_TrafficIconMaterial, 0, 1.0f);
                    break;
                case ACTOR_PEDESTRIANS:
                    m_IGraph->SetWorldMatrix(frame->m_mWorldMat);
                    m_3DDriver->DrawSprite(pos, m_PedsIconMaterial, 0, 1.0f);
                    break;
                }
            }
        }

        if(m_DrawCityParts) {
            for(CityPart& part: m_CacheParts) {
                float dist = (m_CameraPos - part.frame->GetWorldPos()).Magnitude();
                if(dist <= part.sphereRadius + 256) {
                    S_matrix mat;
                    mat.Identity();
                    m_IGraph->SetWorldMatrix(mat);
                    if(part.frame == m_SelectedFrame) {
                        DrawWireframeBox(part.bbox, {1.0f, 0.45f, 0.45f}, 0);
                    } else {
                        DrawWireframeBox(part.bbox, {0.90f, 0.64f, 0.06f}, 0);
                    }
                }
            }
        }

        if(m_DrawCollisions) {
            for(AABBCollider* aabb: m_ColManager.aabbs) {
                S_vector pos = (aabb->min + aabb->max) * 0.5f;

                float dist = (m_CameraPos - pos).Magnitude();
                if(dist <= 256) {
                    S_matrix mat;
                    mat.Identity();

                    m_IGraph->SetWorldMatrix(mat);
                    I3D_bbox bbox;
                    bbox.min = aabb->min;
                    bbox.max = aabb->max;
                    DrawWireframeBox(bbox, g_AABBColor, 0x00);
                }
            }
            for(XTOBBCollider* xtobb: m_ColManager.xtobbs) {
                S_vector pos = *(S_vector*)&xtobb->transform.m_41;
                float dist = (m_CameraPos - pos).Magnitude();
                if(dist <= 256) {
                    m_IGraph->SetWorldMatrix(xtobb->transform);
                    I3D_bbox bbox;
                    bbox.min = xtobb->minExtent;
                    bbox.max = xtobb->maxExtent;
                    DrawWireframeBox(bbox, g_XTOBBColor, 0x00);
                }
            }
            for(OBBCollider* obb: m_ColManager.obbs) {
                S_vector pos = *(S_vector*)&obb->transform.m_41;
                float dist = (m_CameraPos - pos).Magnitude();

                if(dist <= 256) {
                    m_IGraph->SetWorldMatrix(obb->transform);
                    I3D_bbox bbox;
                    bbox.min = obb->minExtent;
                    bbox.max = obb->maxExtent;
                    DrawWireframeBox(bbox, g_OBBColor, 0x00);
                }
            }
            for(SphereCollider* sphere: m_ColManager.spheres) {
                float dist = (m_CameraPos - sphere->pos).Magnitude();

                if(dist <= 256) {
                    S_matrix mat;
                    mat.Identity();

                    I3D_bsphere bsphere;
                    bsphere.pos = sphere->pos;
                    bsphere.radius = sphere->radius;
                    m_3DDriver->DrawSphere(mat, bsphere, g_SphereColor, 0x00);
                }
            }
            for(CylinderCollider* cylinder: m_ColManager.cylinders) {
                S_vector pos = {cylinder->pos.x, 0, cylinder->pos.y};
                float dist = (m_CameraPos - pos).Magnitude();

                if(dist <= 256) {
                    S_matrix mat;
                    mat.Identity();

                    m_IGraph->SetWorldMatrix(mat);
                    DrawWireframeCylinder(pos, cylinder->radius, 512.0f, g_CylinderColor, 0x00);
                }
            }

            /*std::vector<LineVertex> lineVerts;
            std::vector<uint16_t> indices;
            uint16_t vertOffset = 0;

            for(MeshCollider& mesh: m_ColManager.meshes) {
                S_vector pos = mesh.linkedFrame->GetWorldPos();
                float dist = (m_CameraPos - pos).Magnitude();

                if(dist <= 64) {
                    S_matrix worldMat = mesh.linkedFrame->GetWorldMat();

                    for(MeshCollider::Triangle& tri: mesh.tris) {
                        S_vector w0 = tri.positions[0] * worldMat;
                        S_vector w1 = tri.positions[1] * worldMat;
                        S_vector w2 = tri.positions[2] * worldMat;

                        lineVerts.push_back({w0.x, w0.y, w0.z, 0});
                        lineVerts.push_back({w1.x, w1.y, w1.z, 0});
                        lineVerts.push_back({w2.x, w2.y, w2.z, 0});

                        indices.push_back(vertOffset + 0);
                        indices.push_back(vertOffset + 1);
                        indices.push_back(vertOffset + 1);
                        indices.push_back(vertOffset + 2);
                        indices.push_back(vertOffset + 2);
                        indices.push_back(vertOffset + 0);

                        vertOffset += 3;
                    }
                }
            }

            if(!lineVerts.empty()) {
                S_matrix mat;
                mat.Identity();
                m_IGraph->SetWorldMatrix(mat);

                DrawBatchedLines(lineVerts, indices, g_FaceColor, 0x00);
            }*/
        }
        m_IGraph->SetState(ZENABLE, 0);

        if(m_SelectedFrame) {
            m_IGraph->SetState(ZENABLE, 1);
            m_IGraph->SetWorldMatrix(m_SelectedFrame->m_mWorldMat);
            //m_3DDriver->DrawBox(m_SelectedFrame->m_sLocalBVol.bbox, {0, 0, 0}, {1, 0, 0}, 180);
            DrawWireframeBox(m_SelectedFrame->m_sLocalBVol.bbox, {1, 0, 0}, 0);
            m_IGraph->SetState(ZENABLE, 0);

            std::vector<LineVertex> lineVerts;
            std::vector<uint16_t> indices;
            uint16_t vertOffset = 0;

            MeshCollider* mesh = GetMeshCollider(m_SelectedFrame);

            if(mesh) {
                S_vector pos = m_SelectedFrame->GetWorldPos();
                float dist = (m_CameraPos - pos).Magnitude();

                if(dist <= 256) {
                    S_matrix worldMat = m_SelectedFrame->GetWorldMat();

                    for(TriangleCollider* tri: mesh->tris) {
                        S_vector w0 = tri->vertices[0].vertexPos * worldMat;
                        S_vector w1 = tri->vertices[1].vertexPos * worldMat;
                        S_vector w2 = tri->vertices[2].vertexPos * worldMat;

                        lineVerts.push_back({w0.x, w0.y, w0.z, 0});
                        lineVerts.push_back({w1.x, w1.y, w1.z, 0});
                        lineVerts.push_back({w2.x, w2.y, w2.z, 0});

                        indices.push_back(vertOffset + 0);
                        indices.push_back(vertOffset + 1);
                        indices.push_back(vertOffset + 1);
                        indices.push_back(vertOffset + 2);
                        indices.push_back(vertOffset + 2);
                        indices.push_back(vertOffset + 0);

                        vertOffset += 3;
                    }
                }

                if(!lineVerts.empty()) {
                    S_matrix mat;
                    mat.Identity();
                    m_IGraph->SetWorldMatrix(mat);

                    DrawBatchedLines(lineVerts, indices, g_FaceColor, 0x00);
                }
            }
        }
    }

    ImGui_ImplWin32_NewFrame();
    ImGui_ImplDX8_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    if(!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        if(ImGui::IsKeyDown(ImGuiKey_LeftControl) && ImGui::IsKeyPressed(ImGuiKey_S)) { Save(m_MissionPath); }
        if(ImGui::IsKeyDown(ImGuiKey_LeftControl) && ImGui::IsKeyPressed(ImGuiKey_C)) {
            if(m_SelectedFrame) { CopyFrame(m_SelectedFrame); }
        }
        if(ImGui::IsKeyDown(ImGuiKey_LeftControl) && ImGui::IsKeyPressed(ImGuiKey_V)) { PasteFrame(); }
        if(ImGui::IsKeyDown(ImGuiKey_LeftControl) && ImGui::IsKeyPressed(ImGuiKey_D)) {
            if(m_SelectedFrame) {
                CopyFrame(m_SelectedFrame);
                PasteFrame();
            }
        }
    }

    if(g_Editor->GetSettings()->video.fullscreen) { ImGui::RenderMainMenuBar(m_MenuBar, m_IGraph->GetMainHWND()); }

    m_ScriptEditor.Render();

    if(ImGui::Begin("Collection", 0, ImGuiWindowFlags_NoCollapse)) {
        if(m_Hierarchy.size() > 0) {
            if(ImGui::BeginTabBar("CollectionTabs")) {
                if(ImGui::BeginTabItem("Scene")) {
                    if(ImGui::BeginChild("#SceneFrames")) {
                        ShowFrameEntry(m_Hierarchy[0]);
                        ShowFrameEntry(m_Hierarchy[1]);
                        ImGui::EndChild();
                    }
                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("City Parts")) {
                    bool wasClicked = false;
                    bool wasDoubleClicked = false;

                    if(m_CacheParts.empty()) {
                        ImGui::Text("There are no city parts");
                    } else {
                        if(ImGui::BeginChild("#CityParts")) {
                            for(auto& part: m_CacheParts) {
                                if(part.models.empty()) {
                                } else {
                                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                                    if(m_SelectedCityPart == &part) { flags |= ImGuiTreeNodeFlags_Selected; }

                                    bool isOpen = ImGui::TreeNodeEx(part.name.c_str(), flags);

                                    // Check for clicks and double-clicks
                                    if(ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                        ClearSelection();
                                        m_SelectionType = SEL_PART;
                                        m_SelectedCityPart = &part;
                                    }

                                    if(isOpen) {
                                        for(auto frame: part.models) {
                                            if(ImGui::Selectable(frame->GetName(), m_SelectedFrame == frame)) { SelectFrame(frame); }

                                            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                                MoveCameraToTarget(m_SelectedFrame->GetWorldPos());
                                            }
                                        }

                                        ImGui::TreePop();
                                    }
                                }
                            }

                            ImGui::EndChild();
                        }
                    }

                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("Road")) {
                    if(ImGui::BeginTabBar("RoadTabs")) {
                        if(ImGui::BeginTabItem("Waypoints")) {
                            if(m_RoadWaypoints.empty()) {
                                ImGui::Text("There are no waypoints");
                            } else {
                                if(ImGui::BeginChild("#Waypoints")) {
                                    for(auto& pair: m_RoadWaypoints) {
                                        auto& waypoint = pair.second;

                                        char name[128];
                                        sprintf(name, "Waypoint %u [%.1f, %.1f, %.1f]", pair.first, waypoint.pos.x, waypoint.pos.y, waypoint.pos.z);
                                        if(ImGui::Selectable(name, m_SelectedWaypoint == &waypoint)) {
                                            ClearSelection();
                                            m_SelectionType = SEL_WAYPOINT;
                                            m_SelectedWaypoint = &waypoint;
                                            m_SelectedWaypointID = pair.first;
                                        }

                                        if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                            MoveCameraToTarget(m_SelectedWaypoint->pos);
                                        }
                                    }

                                    ImGui::EndChild();
                                }
                            }
                            ImGui::EndTabItem();
                        }

                        if(ImGui::BeginTabItem("Crosspoints")) {
                            if(m_RoadCrosspoints.empty()) {
                                ImGui::Text("There are no crosspoints");
                            } else {
                                if(ImGui::BeginChild("#Crosspoints")) {
                                    for(auto& pair: m_RoadCrosspoints) {
                                        auto& crosspoint = pair.second;
                                        char name[128];
                                        sprintf(name, "Crosspoint %u [%.1f, %.1f, %.1f]", pair.first, crosspoint.pos.x, crosspoint.pos.y, crosspoint.pos.z);
                                        if(ImGui::Selectable(name, m_SelectedCrosspoint == &crosspoint)) {
                                            ClearSelection();
                                            m_SelectionType = SEL_CROSSPOINT;
                                            m_SelectedCrosspoint = &crosspoint;
                                            m_SelectedCrosspointID = pair.first;
                                        }

                                        if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                            MoveCameraToTarget(m_SelectedCrosspoint->pos);
                                        }
                                    }

                                    ImGui::EndChild();
                                }
                            }
                            ImGui::EndTabItem();
                        }

                        ImGui::EndTabBar();
                    }
                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("Web Nodes")) {
                    if(m_WebNodes.empty()) {
                        ImGui::Text("There are no web nodes");
                    } else {
                        if(ImGui::BeginChild("#WebNodes")) {
                            for(auto& pair: m_WebNodes) {
                                auto& node = pair.second;
                                char name[128];
                                sprintf(name, "Node %u [%.1f, %.1f, %.1f]", pair.first, node.pos.x, node.pos.y, node.pos.z);
                                if(ImGui::Selectable(name, m_SelectedNode == &node)) {
                                    ClearSelection();
                                    m_SelectionType = SEL_NODE;
                                    m_SelectedNode = &node;
                                    m_SelectedNodeID = pair.first;
                                }

                                if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { MoveCameraToTarget(m_SelectedNode->pos); }
                            }

                            ImGui::EndChild();
                        }
                    }

                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("Scripts")) {
                    if(ImGui::BeginChild("#Scripts")) {
                        if(ImGui::BeginTabBar("#ScriptTypes")) {
                            if(ImGui::BeginTabItem("Init Scripts")) {
                                if(ImGui::BeginChild("#InitScripts")) {
                                    for(auto& script: m_Scripts) {
                                        if(ImGui::Selectable(script.name.c_str(), m_SelectedScript == &script)) {
                                            ClearSelection();
                                            m_SelectionType = SEL_SCRIPT;
                                            m_SelectedScript = &script;
                                        }

                                        if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                            m_ScriptEditor.Open(m_SelectedScript);
                                        }
                                    }

                                    ImGui::EndChild();
                                }

                                ImGui::EndTabItem();
                            }

                            if(ImGui::BeginTabItem("Scripted Actors")) {
                                if(ImGui::BeginChild("#ScriptedActors")) {
                                    for(auto actor: m_Actors) {
                                        switch(actor->GetType()) {
                                        case ACTOR_ENEMY:
                                        case ACTOR_DETECTOR: {
                                            if(ImGui::Selectable(actor->GetScript()->name.c_str(), m_SelectedScript == actor->GetScript())) {
                                                ClearSelection();
                                                m_SelectionType = SEL_SCRIPT;
                                                m_SelectedScript = actor->GetScript();
                                            }

                                            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                                m_ScriptEditor.Open(m_SelectedScript);
                                            }
                                        } break;
                                        default: break;
                                        }
                                    }

                                    ImGui::EndChild();
                                }

                                ImGui::EndTabItem();
                            }

                            ImGui::EndTabBar();
                        }

                        ImGui::EndChild();
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        } else {
            ImGui::Text("No scene is loaded");
        }

        ImGui::End();
    }

    if(m_SelectionType != SEL_NONE) {
        if(ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            if(ImGui::IsKeyDown(ImGuiKey_LeftControl)) {
                if(m_SelectionType == SEL_FRAME && GetActor(m_SelectedFrame) != nullptr) {
                    DeleteActor(m_SelectedFrame);
                } else {
                    ShowPopupMessage("The selected frame doesn't have an actor!");
                }
            } else {
                DeleteSelection();
            }
        }
    }

    if(ImGui::Begin("Inspector", 0, ImGuiWindowFlags_NoCollapse)) {
        switch(m_SelectionType) {
        case SEL_PART: {
            if(m_SelectedCityPart) {
                ImGui::NonCollapsingHeader("City Part");
                ImGui::Text("Frame: %s", m_SelectedCityPart->frame ? m_SelectedCityPart->frame->GetName() : "None");
                if(m_SelectedCityPart->frame) {
                    ImGui::SameLine();
                    if(ImGui::Button("Select")) { SelectFrame(m_SelectedCityPart->frame); }
                }
            }
        } break;
        case SEL_SCRIPT: {
            ImGui::NonCollapsingHeader("Script");
            ImGui::InputText("Name", &m_SelectedScript->name);
            if(ImGui::Button("Open")) { m_ScriptEditor.Open(m_SelectedScript); }
        } break;
        case SEL_WAYPOINT: {
            if(m_SelectedWaypoint) {
                ImGui::NonCollapsingHeader("Waypoint");
                ImGui::DragFloat3("Position", &m_SelectedWaypoint->pos.x, 0.25f);
                ImGui::DragFloat("Speed", &m_SelectedWaypoint->speed, 0.25f, 0, 1000, "%.1f km/h");

                ImGui::PushID(0);
                ImGui::Text("Next waypoint: ");
                ImGui::SameLine();
                if(m_SelectedWaypoint->nextWaypoint != 0xFFFF) {
                    bool isWaypoint = (m_SelectedWaypoint->nextWaypoint & 0x8000) != 0;
                    if(isWaypoint) {
                        int id = m_SelectedWaypoint->nextWaypoint & 0x7FFF;
                        char name[128];
                        sprintf(name, "Waypoint %d", id);

                        if(ImGui::Button(name)) {
                            ClearSelection();
                            SelectWaypoint(&m_RoadWaypoints[id]);
                            m_SelectedWaypointID = id;
                        }

                    } else {
                        char name[128];
                        sprintf(name, "Crosspoint %d", m_SelectedWaypoint->nextWaypoint);

                        if(ImGui::Button(name)) {
                            ClearSelection();
                            SelectCrosspoint(&m_RoadCrosspoints[m_SelectedWaypoint->nextWaypoint]);
                            m_SelectedCrosspointID = m_SelectedWaypoint->nextWaypoint;
                        }
                    }
                } else {
                    ImGui::Text("None");
                }
                ImGui::SameLine();
                if(ImGui::Button("Browse")) {
                    m_WaypointSelectionType = WPS_NEXTWP;
                    m_ReferenceSelectionType = SEL_WAYPOINT;
                    m_ShowingRefSelection = true;
                }
                ImGui::SameLine();
                if(ImGui::Button("New")) {
                    auto point = CreateWaypoint();

                    S_vector forwardDir = CalculateWaypointForwardDirection(*m_SelectedWaypoint);

                    point->pos = m_SelectedWaypoint->pos + (forwardDir * 3.5f);
                    point->prevWaypoint = m_SelectedWaypointID | 0x8000;
                    point->prevCrosspoint = m_SelectedWaypoint->prevCrosspoint;
                    point->nextCrosspoint = m_SelectedWaypoint->nextCrosspoint;
                    m_SelectedWaypoint->nextWaypoint = m_HighestWaypointID | 0x8000;
                }
                ImGui::PopID();

                ImGui::PushID(1);
                ImGui::Text("Prev waypoint: ");
                ImGui::SameLine();
                if(m_SelectedWaypoint->prevWaypoint != 0xFFFF) {
                    bool isWaypoint = (m_SelectedWaypoint->prevWaypoint & 0x8000) != 0;
                    if(isWaypoint) {
                        int id = m_SelectedWaypoint->prevWaypoint & 0x7FFF;
                        char name[128];
                        sprintf(name, "Waypoint %d", id);

                        if(ImGui::Button(name)) {
                            ClearSelection();
                            SelectWaypoint(&m_RoadWaypoints[id]);
                            m_SelectedWaypointID = id;
                        }

                    } else {
                        char name[128];
                        sprintf(name, "Crosspoint %d", m_SelectedWaypoint->prevWaypoint);
                        if(ImGui::Button(name)) {
                            ClearSelection();
                            SelectCrosspoint(&m_RoadCrosspoints[m_SelectedWaypoint->prevWaypoint]);
                            m_SelectedCrosspointID = m_SelectedWaypoint->prevWaypoint;
                        }
                    }
                } else {
                    ImGui::Text("None");
                }
                ImGui::SameLine();
                if(ImGui::Button("Browse")) {
                    m_WaypointSelectionType = WPS_PREVWP;
                    m_ReferenceSelectionType = SEL_WAYPOINT;
                    m_ShowingRefSelection = true;
                }
                ImGui::SameLine();
                if(ImGui::Button("New")) {
                    auto point = CreateWaypoint();

                    S_vector forwardDir = CalculateWaypointForwardDirection(*m_SelectedWaypoint);

                    point->pos = m_SelectedWaypoint->pos + ((forwardDir * -1) * 3.5f);
                    point->nextWaypoint = m_SelectedWaypointID | 0x8000;
                    point->prevCrosspoint = m_SelectedWaypoint->prevCrosspoint;
                    point->nextCrosspoint = m_SelectedWaypoint->nextCrosspoint;
                    m_SelectedWaypoint->prevWaypoint = m_HighestWaypointID | 0x8000;
                }
                ImGui::PopID();

                ImGui::PushID(2);
                ImGui::Text("Next crosspoint: ");
                ImGui::SameLine();
                if(m_SelectedWaypoint->nextCrosspoint != 0xFFFF) {
                    char name[128];
                    sprintf(name, "Crosspoint %d", m_SelectedWaypoint->nextCrosspoint);
                    if(ImGui::Button(name)) {
                        ClearSelection();
                        SelectCrosspoint(&m_RoadCrosspoints[m_SelectedWaypoint->nextCrosspoint]);
                        m_SelectedCrosspointID = m_SelectedWaypoint->nextCrosspoint;
                    }
                } else {
                    ImGui::Text("None");
                }
                ImGui::SameLine();
                if(ImGui::Button("Browse")) {
                    m_WaypointSelectionType = WPS_NEXTCP;
                    m_ReferenceSelectionType = SEL_WAYPOINT;
                    m_ShowingRefSelection = true;
                }
                ImGui::PopID();

                ImGui::PushID(3);
                ImGui::Text("Prev crosspoint: ");
                ImGui::SameLine();
                if(m_SelectedWaypoint->prevCrosspoint != 0xFFFF) {
                    char name[128];
                    sprintf(name, "Crosspoint %d", m_SelectedWaypoint->prevCrosspoint);
                    if(ImGui::Button(name)) {
                        ClearSelection();
                        SelectCrosspoint(&m_RoadCrosspoints[m_SelectedWaypoint->prevCrosspoint]);
                        m_SelectedCrosspointID = m_SelectedWaypoint->prevCrosspoint;
                    }
                } else {
                    ImGui::Text("None");
                }
                ImGui::SameLine();
                if(ImGui::Button("Browse")) {
                    m_WaypointSelectionType = WPS_PREVCP;
                    m_ReferenceSelectionType = SEL_WAYPOINT;
                    m_ShowingRefSelection = true;
                }
                ImGui::PopID();

                if(m_DrawRoadPoints) {
                    D3DMATRIX proj, view;
                    m_D3DDevice->GetTransform(D3DTS_PROJECTION, &proj);
                    m_D3DDevice->GetTransform(D3DTS_VIEW, &view);

                    S_vector euler = {0, 0, 0}, scale = {1, 1, 1};

                    S_matrix mat;
                    PackTransformComponents(&mat, &m_SelectedWaypoint->pos, &euler, &scale);

                    ImGuiIO& io = ImGui::GetIO();
                    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
                    ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::TRANSLATE, ImGuizmo::WORLD, mat.e);

                    if(ImGuizmo::IsUsing() && IsMouseDeltaSignificant(0.01f)) {
                        ExtractTransformComponents(&mat, &m_SelectedWaypoint->pos, &euler, &scale);
                        if(m_TransformGridSnapping) m_SelectedWaypoint->pos = SnapToGrid(m_SelectedWaypoint->pos);
                    }
                }
            }
        } break;
        case SEL_CROSSPOINT: {
            if(m_SelectedCrosspoint) {
                ImGui::NonCollapsingHeader("Crosspoint");
                ImGui::DragFloat3("Position", &m_SelectedCrosspoint->pos.x, 0.25f);
                ImGui::Checkbox("Has semaphore", &m_SelectedCrosspoint->hasSemaphore);
                ImGui::DragFloat("Speed", &m_SelectedCrosspoint->speed, 0.25f, 0, 1000, "%.1f km/h");

                if(ImGui::TreeNode("Waypoint links")) {
                    for(int i = 0; i < 4; i++) {
                        if(m_SelectedCrosspoint->waypointLinks[i] != UINT16_MAX) {
                            bool isWaypoint = (m_SelectedCrosspoint->waypointLinks[i] & 0x8000) != 0;
                            if(isWaypoint) {
                                int id = m_SelectedCrosspoint->waypointLinks[i] & 0x7FFF;

                                char name[128];
                                sprintf(name, "Waypoint %d", id);
                                ImGui::Text("Waypoint link %d: ", i);
                                ImGui::SameLine();
                                if(ImGui::Button(name)) {
                                    ClearSelection();
                                    SelectWaypoint(&m_RoadWaypoints[id]);
                                    m_SelectedWaypointID = id;
                                }
                            } else {
                                char name[128];
                                sprintf(name, "Crosspoint %d", m_SelectedCrosspoint->waypointLinks[i]);
                                ImGui::Text("Waypoint link %d: ", i);
                                ImGui::SameLine();
                                if(ImGui::Button(name)) {
                                    ClearSelection();
                                    SelectCrosspoint(&m_RoadCrosspoints[m_SelectedCrosspoint->waypointLinks[i]]);
                                    m_SelectedCrosspointID = m_SelectedCrosspoint->waypointLinks[i];
                                }
                            }
                        } else {
                            ImGui::Text("Waypoint link %d: None", i);
                        }
                        ImGui::SameLine();
                        if(ImGui::Button("Browse")) {
                            m_ReferenceSelectionType = SEL_WAYPOINT;
                            m_WaypointSelectionType = WPS_WPLINK;
                            m_ReferencedSlotID = i;
                            m_ShowingRefSelection = true;
                        }
                        ImGui::SameLine();
                        if(ImGui::Button("New")) {
                            auto point = CreateWaypoint();

                            S_vector forwardDir = CalculateWaypointForwardDirection(*m_SelectedWaypoint);

                            point->pos = m_SelectedWaypoint->pos + (forwardDir * 3.5f);
                            point->prevWaypoint = m_SelectedCrosspointID;
                            point->prevCrosspoint = m_SelectedCrosspointID;
                            m_SelectedCrosspoint->waypointLinks[i] = m_HighestWaypointID | 0x8000;
                        }
                    }
                    ImGui::TreePop();
                }

                if(ImGui::TreeNode("Direction links")) {
                    for(int i = 0; i < 4; i++) {
                        RoadDirectionLink& link = m_SelectedCrosspoint->directionLinks[i];

                        if(link.crosspointLink != 0xFFFF) {
                            char name[128];
                            sprintf(name, "Direction link %d", i);
                            if(ImGui::TreeNode(name)) {
                                sprintf(name, "Crosspoint %d", link.crosspointLink);
                                ImGui::Text("Crosspoint link %d: ", i);
                                ImGui::SameLine();
                                if(ImGui::Button(name)) {
                                    ClearSelection();
                                    SelectCrosspoint(&m_RoadCrosspoints[link.crosspointLink]);
                                    m_SelectedCrosspointID = link.crosspointLink;
                                }
                                ImGui::SameLine();
                                if(ImGui::Button("Browse")) {
                                    m_ReferenceSelectionType = SEL_CROSSPOINT;
                                    m_WaypointSelectionType = WPS_CPLINK;
                                    m_ShowingRefSelection = true;
                                }

                                ImGui::Text("Distance: %.1f", link.distance);
                                ImGui::SliderFloat("Angle", &link.angle, 0.0f, 360.0f);
                                ImGui::SliderInt("Priority", (int*)&link.priority, 0, 100);

                                if(ImGui::TreeNode("Lanes")) {
                                    for(int j = 0; j < 4; j++) {
                                        static const char* laneTypes[] = {"Disabled", "AI / Disabled for traffic", "Traffic", "Longitudinal parking"};

                                        RoadLane& lane = link.lanes[j];
                                        sprintf(name, "Lane %d", j);
                                        if(ImGui::TreeNode(name)) {
                                            static int type = 0;

                                            type = lane.type;

                                            if(ImGui::Combo("Type", &type, laneTypes, IM_ARRAYSIZE(laneTypes))) { lane.type = type; }

                                            ImGui::DragFloat("Distance", &lane.distance, 0.25f);
                                            ImGui::TreePop();
                                        }
                                    }

                                    ImGui::TreePop();
                                }

                                ImGui::TreePop();
                            }
                        } else {
                            ImGui::Text("Direction link %d: None", i);
                            ImGui::SameLine();
                            if(ImGui::Button("Browse")) {
                                m_ReferenceSelectionType = SEL_CROSSPOINT;
                                m_WaypointSelectionType = WPS_CPLINK;
                                m_ReferencedSlotID = i;
                                m_ShowingRefSelection = true;
                            }
                        }
                    }

                    ImGui::TreePop();
                }

                if(m_DrawRoadPoints) {
                    D3DMATRIX proj, view;
                    m_D3DDevice->GetTransform(D3DTS_PROJECTION, &proj);
                    m_D3DDevice->GetTransform(D3DTS_VIEW, &view);

                    S_vector euler = {0, 0, 0}, scale = {1, 1, 1};

                    S_matrix mat;
                    PackTransformComponents(&mat, &m_SelectedCrosspoint->pos, &euler, &scale);

                    ImGuiIO& io = ImGui::GetIO();
                    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
                    ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::TRANSLATE, ImGuizmo::WORLD, mat.e);

                    if(ImGuizmo::IsUsing() && IsMouseDeltaSignificant(0.01f)) {
                        ExtractTransformComponents(&mat, &m_SelectedCrosspoint->pos, &euler, &scale);

                        if(m_TransformGridSnapping) m_SelectedCrosspoint->pos = SnapToGrid(m_SelectedCrosspoint->pos);
                    }
                }
            }
            break;
        }
        case SEL_NODE: {
            if(m_SelectedNode) {
                ImGui::NonCollapsingHeader("Web Node");

                static int selType = 0;

                static const char* typeNames[] =
                    {"Pedestrian", "AI", "Traffic", "Tram Station", "Railway Onboard", "Railway Waypoint", "Railway Stop", "Tram Unknown", "AI Unknown"};

                ImGui::DragFloat3("Position", &m_SelectedNode->pos.x, 0.25f);

                switch(m_SelectedNode->type) {
                case WPT_Pedestrian: selType = 0; break;
                case WPT_AI: selType = 1; break;
                case WPT_Traffic: selType = 2; break;
                case WPT_TramStation: selType = 3; break;
                case WPT_Special: selType = 4; break;
                case WPT_RailwayOnboard: selType = 5; break;
                case WPT_RailwayWaypoint: selType = 6; break;
                case WPT_RailwayStop: selType = 7; break;
                case WPT_TramUnknown: selType = 8; break;
                case WPT_AIUnknown: selType = 9; break;
                }

                if(ImGui::Combo("Type", &selType, typeNames, IM_ARRAYSIZE(typeNames))) {
                    switch(selType) {
                    case 0: m_SelectedNode->type = WPT_Pedestrian; break;
                    case 1: m_SelectedNode->type = WPT_AI; break;
                    case 2: m_SelectedNode->type = WPT_Traffic; break;
                    case 3: m_SelectedNode->type = WPT_TramStation; break;
                    case 4: m_SelectedNode->type = WPT_Special; break;
                    case 5: m_SelectedNode->type = WPT_RailwayOnboard; break;
                    case 6: m_SelectedNode->type = WPT_RailwayWaypoint; break;
                    case 7: m_SelectedNode->type = WPT_RailwayStop; break;
                    case 8: m_SelectedNode->type = WPT_TramUnknown; break;
                    case 9: m_SelectedNode->type = WPT_AIUnknown; break;
                    }
                }

                ImGui::InputUInt16("ID", &m_SelectedNode->id);
                ImGui::InputUInt16("Radius", &m_SelectedNode->radius);

                std::vector<int> toRemove;

                ImGui::Text("Links");
                ImGui::Separator();
                if(ImGui::BeginChild("#Links")) {
                    int i = 0;

                    if(m_SelectedNode->links.empty()) {
                        ImGui::Text("Empty...");
                    } else {
                        for(auto& link: m_SelectedNode->links) {
                            ImGui::PushID(i);
                            ImGui::Text("%d", i);
                            ImGui::SameLine();
                            if(ImGui::Button("Remove")) { toRemove.push_back(i); }
                            ImGui::Separator();
                            ImGui::Text("Destination Node: ");
                            ImGui::SameLine();
                            if(link.endPoint != 0xFFFF) {
                                char name[128];
                                sprintf(name, "Node %u", link.endPoint);
                                if(ImGui::Button(name)) {
                                    ClearSelection();
                                    SelectWebNode(&m_WebNodes[link.endPoint]);
                                    m_SelectedNodeID = link.endPoint;
                                }
                            } else {
                                ImGui::Text("None");
                            }
                            ImGui::SameLine();
                            if(ImGui::Button("Browse")) {
                                m_ReferenceSelectionType = SEL_NODE;
                                m_ReferencedSlotID = i;
                                m_ShowingRefSelection = true;
                            }
                            ImGui::SameLine();
                            if(ImGui::Button("New")) {
                                WebNode* node = CreateWebNode();

                                SelectWebNode(node);
                                m_SelectedNodeID = m_HighestNodeID;
                            }

                            static const char* connTypeNames[] = {"Pedestrian", "AI", "Traffic Forward", "Railway", "Traffic Backward", "Other"};

                            switch(link.type) {
                            case WCT_Pedestrian: selType = 0; break;
                            case WCT_AI: selType = 1; break;
                            case WCT_TrafficForward: selType = 2; break;
                            case WCT_Railway: selType = 3; break;
                            case WCT_TrafficBackward: selType = 4; break;
                            case WCT_Other: selType = 5; break;
                            }

                            if(ImGui::Combo("Type", &selType, connTypeNames, IM_ARRAYSIZE(connTypeNames))) {
                                switch(selType) {
                                case 0: link.type = WCT_Pedestrian; break;
                                case 1: link.type = WCT_AI; break;
                                case 2: link.type = WCT_TrafficForward; break;
                                case 3: link.type = WCT_Railway; break;
                                case 4: link.type = WCT_TrafficBackward; break;
                                case 5: link.type = WCT_Other; break;
                                }
                            }

                            ImGui::Separator();
                            ImGui::NewLine();
                            ImGui::PopID();
                            i++;
                        }
                    }

                    if(ImGui::Button("Add")) {
                        WebConnection& conn = m_SelectedNode->links.emplace_back();
                        conn.endPoint = 0xFFFF;
                        conn.type = WCT_AI;
                        conn.length = 0.0f;
                    }

                    ImGui::EndChild();
                }
                ImGui::Separator();

                if(m_DrawWebNodes) {
                    D3DMATRIX proj, view;
                    m_D3DDevice->GetTransform(D3DTS_PROJECTION, &proj);
                    m_D3DDevice->GetTransform(D3DTS_VIEW, &view);

                    S_vector euler = {0, 0, 0}, scale = {1, 1, 1};

                    S_matrix mat;
                    PackTransformComponents(&mat, &m_SelectedNode->pos, &euler, &scale);

                    ImGuiIO& io = ImGui::GetIO();
                    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
                    ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::TRANSLATE, ImGuizmo::WORLD, mat.e);

                    if(ImGuizmo::IsUsing() && IsMouseDeltaSignificant(0.01f)) {
                        ExtractTransformComponents(&mat, &m_SelectedNode->pos, &euler, &scale);

                        if(m_TransformGridSnapping) m_SelectedNode->pos = SnapToGrid(m_SelectedNode->pos);
                    }
                }
            }
        } break;
        case SEL_FRAME: {
            if(m_SelectedFrame) {
                ImGui::NonCollapsingHeader("Frame");
                if(m_SelectedFrame == m_PrimarySector || m_SelectedFrame == m_BackdropSector) {
                    ImGui::Text("Name: %s", m_SelectedFrameName);
                } else {
                    ImGui::InputText("Name", m_SelectedFrameName, 256);
                }
                ImGui::Text("Type: %s", GetFrameTypeName(m_SelectedFrame->GetType()));
                if(m_SelectedFrame->GetType() == FRAME_VISUAL) {
                    I3D_visual* visual = (I3D_visual*)m_SelectedFrame;
                    ImGui::Text("Visual type: %s", GetVisualTypeName(visual->GetVisualType()));
                }

                ImGui::Separator();

                if(ImGui::DragFloat3("Position", (float*)&m_SelectedFramePos, 0.25f)) {
                    m_SelectedFrame->SetPos(m_SelectedFramePos);
                    m_SelectedFrameWorldPos = m_SelectedFrame->GetWorldPos();
                }
                ImGui::DragFloat3("Rotation", (float*)&m_SelectedFrameEuler, 0.25f);
                ImGui::DragFloat3("Scale", (float*)&m_SelectedFrameScale, 0.25f);

                switch(m_SelectedFrame->GetType()) {
                case FRAME_MODEL: {
                    static bool validModel = true;
                    validModel = m_SelectedFrame->m_pSzModelName && (strlen(m_SelectedFrame->m_pSzModelName) > 0);
                    I3D_model* model = (I3D_model*)m_SelectedFrame;

                    ImGui::NonCollapsingHeader("Model");

                    ImGui::Text("File: %s", g_ModelsMap[model].c_str());
                    ImGui::SameLine();
                    if(ImGui::Button("Browse")) {
                        std::string path = OpenFileDialog(FileDialog::Mode::OpenFile, "Select model file", {{"4DS Model File (*.4ds)", "*.4ds"}});

                        if(!path.empty()) {
                            std::string fileName = "Models\\" + ProcessFileName(path, "i3d");
                            validModel = I3D_SUCCESS(model->Open(fileName.c_str()));

                            if(validModel) { g_ModelsMap[model] = ProcessFileName(path, "i3d"); }
                        }
                    }

                    if(!validModel) { ImGui::TextColored({0.85f, 0.15f, 0.15f, 1.0f}, "Invalid model file!"); }
                } break;
                case FRAME_CAMERA: {
                    I3D_camera* cam = (I3D_camera*)m_SelectedFrame;

                    m_IGraph->SetWorldMatrix(cam->m_mWorldMat);
                    m_3DDriver->DrawSprite(m_SelectedFramePos, m_CameraIconMaterial, 0, 1.0f);

                    S_matrix mat;
                    mat.Identity();
                    m_IGraph->SetWorldMatrix(mat);

                    DrawWireframeFrustum(m_SelectedFramePos, cam->GetDir(), 1.0f, 0.5f, 4.0f, 2.0f, 5.0f, {1.0f, 1.0f, 1.0f}, 0);
                } break;
                case FRAME_LIGHT: {
                    static const char* lightTypeNames[] = {"Point", "Spot", "Directional", "Ambient", "Fog", "Point Ambient", "Point Fog", "Layered Fog"};
                    static int selectedLightIndex = 0;

                    I3D_light* light = (I3D_light*)m_SelectedFrame;

                    m_IGraph->SetWorldMatrix(light->m_mWorldMat);
                    m_3DDriver->DrawSprite(m_SelectedFramePos, m_LightIconMaterial, 0, 1.0f);

                    ImGui::NonCollapsingHeader("Light");

                    static I3D_LIGHTTYPE type;
                    static S_vector color;
                    static float power;
                    static uint32_t mode;
                    static S_vector2 range;
                    static S_vector2 angle;

                    type = light->GetLightType();
                    color = light->GetColor();
                    power = light->GetPower();
                    mode = light->GetMode();
                    light->GetCone(angle.x, angle.y);
                    angle.x = DEG(angle.x);
                    angle.y = DEG(angle.y);

                    light->GetRange(range.x, range.y);
                    if(selectedLightIndex != (light->GetLightType() - 1)) { selectedLightIndex = (light->GetLightType() - 1); }

                    if(ImGui::Combo("Type", &selectedLightIndex, lightTypeNames, IM_ARRAYSIZE(lightTypeNames))) {
                        I3D_LIGHTTYPE lightType = (I3D_LIGHTTYPE)(selectedLightIndex + 1);
                        light->SetLightType(lightType);
                    }

                    S_matrix mat;
                    mat.Identity();
                    m_IGraph->SetWorldMatrix(mat);
                    if(type == LIGHT_POINT) {
                        I3D_bsphere sphere;
                        sphere.pos = {0, 0, 0};
                        sphere.radius = range.y;
                        m_3DDriver->DrawSphere(light->m_mWorldMat, sphere, color, 0);
                        if(range.x < range.y) {
                            sphere.radius = range.x;
                            m_3DDriver->DrawSphere(light->m_mWorldMat, sphere, {0.75f, 0.75f, 0.75f}, 0);
                        }
                    } else if(type == LIGHT_SPOT) {
                        DrawWireframeCone(light->GetWorldPos(), light->GetDir(), angle.x / 4, 16, color, 0);
                        if(angle.y < angle.x) { DrawWireframeCone(light->GetWorldPos(), light->GetDir(), angle.y / 4, 16, {0.75f, 0.75f, 0.75f}, 0); }
                    }

                    if(ImGui::ColorEdit3("Color", &color.x)) { light->SetColor2(color); }
                    if(ImGui::DragFloat("Power", &power, 0.25f)) { light->SetPower(power); }
                    if(ImGui::InputInt("Mode", (int*)&mode, 1, 1)) { light->SetMode(mode); }
                    if(ImGui::DragFloat2("Range", &range.x, 0.25f)) { light->SetRange(range.x, range.y); }
                    if(ImGui::DragFloat2("Cone angle", &angle.x, 0.25f)) { light->SetCone(RAD(angle.x), RAD(angle.y)); }

                    ImGui::Text("Sectors");
                    ImGui::SameLine();
                    if(ImGui::Button("Add")) {
                        m_ReferenceSelectionType = SEL_FRAME;
                        m_ReferencedFrameType = FRAME_SECTOR;
                        m_ShowingRefSelection = true;
                    }
                    if(light->NumLightSectors() > 0) {
                        ImGui::Separator();
                        if(ImGui::BeginChild("#lightSectors")) {
                            for(int i = light->NumLightSectors(); i--;) {
                                I3D_sector* sector = light->GetLightSectors()[i];

                                ImGui::Text("%s", sector->GetName());
                                ImGui::SameLine();
                                if(ImGui::Button("Select")) { SelectFrame(sector); }
                                ImGui::SameLine();
                                if(ImGui::Button("Remove")) { sector->DeleteLight(light); }
                            }
                            ImGui::EndChild();
                        }
                    }
                } break;
                case FRAME_SOUND: {
                    static const char* soundTypeNames[] = {"Point", "Spot", "Ambient", "Volume", "Point Ambient"};
                    static int selectedSoundIndex = 0;

                    I3D_sound* sound = (I3D_sound*)m_SelectedFrame;

                    m_IGraph->SetWorldMatrix(sound->m_mWorldMat);
                    m_3DDriver->DrawSprite(m_SelectedFramePos, m_SoundIconMaterial, 0, 1.0f);

                    ImGui::NonCollapsingHeader("Sound");

                    static I3D_SOUNDTYPE type;
                    static std::string fileName;
                    static S_vector2 radius;
                    static S_vector2 falloff;
                    static S_vector2 cone;
                    static float volume;
                    static float outVolume;
                    static bool loop;

                    type = sound->GetSoundType();
                    fileName = g_SoundsMap[sound];
                    sound->GetRange(radius.x, radius.y, falloff.x, falloff.y);
                    sound->GetCone(cone.x, cone.y);
                    cone.x = DEG(cone.x);
                    cone.y = DEG(cone.y);
                    volume = sound->GetVolume();
                    outVolume = sound->GetOutVol();
                    loop = sound->IsLoop();

                    if(selectedSoundIndex != (sound->GetSoundType() - 1)) { selectedSoundIndex = (sound->GetSoundType() - 1); }

                    if(ImGui::Combo("Type", &selectedSoundIndex, soundTypeNames, IM_ARRAYSIZE(soundTypeNames))) {
                        I3D_SOUNDTYPE soundType = (I3D_SOUNDTYPE)(selectedSoundIndex + 1);
                        sound->SetSoundType(soundType);
                    }

                    S_matrix mat;
                    mat.Identity();
                    m_IGraph->SetWorldMatrix(mat);
                    if(type == SOUND_POINT) {
                        I3D_bsphere sphere;
                        sphere.pos = {0, 0, 0};
                        sphere.radius = radius.y;
                        m_3DDriver->DrawSphere(sound->m_mWorldMat, sphere, {0.85f, 0.85f, 0.85f}, 0);
                        if(radius.x < radius.y) {
                            sphere.radius = radius.x;
                            m_3DDriver->DrawSphere(sound->m_mWorldMat, sphere, {0.75f, 0.75f, 0.75f}, 0);
                        }
                    } else if(type == SOUND_SPOT) {
                        DrawWireframeCone(sound->GetWorldPos(), sound->GetDir(), cone.x / 4, 16, {0.85f, 0.85f, 0.85f}, 0);
                        if(cone.y < cone.x) { DrawWireframeCone(sound->GetWorldPos(), sound->GetDir(), cone.y / 4, 16, {0.75f, 0.75f, 0.75f}, 0); }
                    }

                    ImGui::InputText("Sound file", &fileName);
                    ImGui::SameLine();
                    if(ImGui::Button("Open")) {
                        if(sound->Open(("Sounds\\" + fileName).c_str(), 0, nullptr, nullptr) == I3D_OK) { g_SoundsMap[sound] = fileName; }
                    }
                    if(ImGui::Button("Browse")) {
                        std::string path = OpenFileDialog(FileDialog::Mode::OpenFile, "Select sound file", {{"Wave Sound File (*.wav)", "*.wav"}});

                        if(!path.empty()) {
                            fileName = ProcessFileName(path, "wav");
                            if(sound->Open(("Sounds\\" + fileName).c_str(), 0, nullptr, nullptr) == I3D_OK) { g_SoundsMap[sound] = fileName; }
                        }
                    }

                    if(ImGui::DragFloat2("Radius", &radius.x, 0.25f)) { sound->SetRange(radius.x, radius.y, falloff.x, falloff.y); }
                    if(ImGui::DragFloat2("Falloff", &falloff.x, 0.025f)) { sound->SetRange(radius.x, radius.y, falloff.x, falloff.y); }
                    if(ImGui::DragFloat2("Cone", &cone.x, 0.25f)) { sound->SetCone(RAD(cone.x), RAD(cone.y)); }
                    if(ImGui::SliderFloat("Volume", &volume, 0, 1)) { sound->SetVolume(volume); }
                    if(ImGui::SliderFloat("Out Volume", &outVolume, 0, 1)) { sound->SetOutVol(outVolume); }
                    if(ImGui::Checkbox("Loop", &loop)) { sound->SetLoop(loop); }

                    ImGui::Text("Sectors");
                    ImGui::SameLine();
                    if(ImGui::Button("Add")) {
                        m_ReferenceSelectionType = SEL_FRAME;
                        m_ReferencedFrameType = FRAME_SECTOR;
                        m_ShowingRefSelection = true;
                    }
                    if(sound->NumSoundSectors() > 0) {
                        ImGui::Separator();
                        if(ImGui::BeginChild("#soundSectors")) {
                            for(int i = sound->NumSoundSectors(); i--;) {
                                I3D_sector* sector = sound->GetSoundSectors()[i];

                                ImGui::Text("%s", sector->GetName());
                                ImGui::SameLine();
                                if(ImGui::Button("Select")) { SelectFrame(sector); }
                                ImGui::SameLine();
                                if(ImGui::Button("Remove")) { sector->DeleteSound(sound); }
                            }
                            ImGui::EndChild();
                        }
                    }
                } break;
                }

                Actor* actor = GetActor(m_SelectedFrame);
                if(actor) {
                    ImGui::NonCollapsingHeader("Actor");
                    ImGui::Text("Type: %s", g_ActorTypeString[actor->GetType()].c_str());
                    actor->OnInspectorGUI();
                }

                HierarchyEntry* entry = FindEntryByFrame(m_SelectedFrame);
                if(entry && !entry->colliders.empty()) {
                    ImGui::NonCollapsingHeader("Collisions");
                    int index = 0;
                    for(Collider* collider: entry->colliders) {
                        char name[64];
                        sprintf(name, "Collision %d", index);
                        if(ImGui::TreeNode(name)) {
                            ImGui::Text("Type: %s", collider->GetTypeAsString().c_str());
                            ImGui::TreePop();
                        }
                        index++;
                    }
                }

                D3DMATRIX proj, view;
                m_D3DDevice->GetTransform(D3DTS_PROJECTION, &proj);
                m_D3DDevice->GetTransform(D3DTS_VIEW, &view);

                S_matrix mat;
                PackTransformComponents(&mat, &m_SelectedFrameWorldPos, &m_SelectedFrameEuler, &m_SelectedFrameScale);

                if(ImGui::IsKeyPressed(ImGuiKey_E) && ImGui::IsKeyDown(ImGuiKey_LeftControl)) { m_CurrentTransformOperation = ImGuizmo::SCALE; }
                if(ImGui::IsKeyPressed(ImGuiKey_R) && ImGui::IsKeyDown(ImGuiKey_LeftControl)) { m_CurrentTransformOperation = ImGuizmo::ROTATE; }
                if(ImGui::IsKeyPressed(ImGuiKey_T) && ImGui::IsKeyDown(ImGuiKey_LeftControl)) { m_CurrentTransformOperation = ImGuizmo::TRANSLATE; }

                ImGuiIO& io = ImGui::GetIO();
                ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
                ImGuizmo::Manipulate((float*)&view, (float*)&proj, m_CurrentTransformOperation, ImGuizmo::LOCAL, mat.e);

                if(ImGuizmo::IsUsing() && IsMouseDeltaSignificant(0.01f)) {
                    ExtractTransformComponents(&mat, &m_SelectedFrameWorldPos, &m_SelectedFrameEuler, &m_SelectedFrameScale);

                    if(m_CurrentTransformOperation == ImGuizmo::TRANSLATE && m_TransformGridSnapping) {
                        m_SelectedFrameWorldPos = SnapToGrid(m_SelectedFrameWorldPos);
                    }
                }

                m_SelectedFrame->SetWorldPos(m_SelectedFrameWorldPos);
                m_SelectedFrame->SetRot(QuatFromEuler(m_SelectedFrameEuler));
                m_SelectedFrame->SetScale(m_SelectedFrameScale);
                m_SelectedFrame->Update();
            }
        } break;
        }

        ImGui::End();
    }

    static char buf[256];
    switch(m_ReferenceSelectionType) {
    case SEL_FRAME:
        switch(m_ReferencedFrameType) {
        case FRAME_SECTOR: sprintf(buf, "Select sector"); break;
        case FRAME_LIGHT: sprintf(buf, "Select light"); break;
        case FRAME_SOUND: sprintf(buf, "Select sound"); break;
        default: sprintf(buf, "Select frame"); break;
        }
        break;
    case SEL_CROSSPOINT: sprintf(buf, "Select crosspoint"); break;
    case SEL_WAYPOINT: sprintf(buf, "Select waypoint"); break;
    case SEL_NODE: sprintf(buf, "Select node"); break;
    }
    if(m_ShowingRefSelection && ImGui::Begin(buf)) {
        if(ImGui::Button("Select")) {
            m_HasSelectedReference = true;
            m_ShowingRefSelection = false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel")) { m_ShowingRefSelection = false; }
        if(ImGui::BeginChild("#RefSelection")) {
            switch(m_ReferenceSelectionType) {
            case SEL_FRAME:
                IterateFrames(m_PrimarySector, m_ReferencedFrameType);
                IterateFrames(m_BackdropSector, m_ReferencedFrameType);
                break;
            case SEL_CROSSPOINT: {
                for(auto& pair: m_RoadCrosspoints) {
                    RoadCrosspoint& cross = pair.second;

                    char buf[32];
                    sprintf(buf, "Crosspoint %u", pair.first);
                    if(ImGui::Selectable(buf, m_ReferencedData == &cross)) { m_ReferencedData = &cross; }
                }
            } break;
            case SEL_WAYPOINT: {
                switch(m_WaypointSelectionType) {
                case WPS_NEXTWP:
                case WPS_PREVWP: {
                    if(ImGui::BeginTabBar("#WPTabs")) {
                        if(ImGui::BeginTabItem("Waypoints")) {
                            if(ImGui::BeginChild("#RefWaypoints")) {
                                for(auto& pair: m_RoadWaypoints) {
                                    char buf[32];
                                    sprintf(buf, "Waypoint %u", pair.first);
                                    if(ImGui::Selectable(buf, m_ReferencedData == &pair)) {
                                        m_SelectingCrosspoint = false;
                                        m_ReferencedData = &pair;
                                    }
                                }
                                ImGui::EndChild();
                            }
                            ImGui::EndTabItem();
                        }

                        if(ImGui::BeginTabItem("Crosspoints")) {
                            if(ImGui::BeginChild("#RefCrosspoints")) {
                                for(auto& pair: m_RoadCrosspoints) {
                                    char buf[32];
                                    sprintf(buf, "Crosspoint %u", pair.first);
                                    if(ImGui::Selectable(buf, m_ReferencedData == &pair)) {
                                        m_SelectingCrosspoint = true;
                                        m_ReferencedData = &pair;
                                    }
                                }
                                ImGui::EndChild();
                            }
                            ImGui::EndTabItem();
                        }

                        ImGui::EndTabBar();
                    }
                } break;
                default: {
                    for(auto& pair: m_RoadWaypoints) {
                        char buf[32];
                        sprintf(buf, "Waypoint %u", pair.first);
                        if(ImGui::Selectable(buf, m_ReferencedData == &pair)) { m_ReferencedData = &pair; }
                    }
                } break;
                }
            } break;
            case SEL_NODE: {
                for(auto& pair: m_WebNodes) {
                    char buf[32];
                    sprintf(buf, "Node %u", pair.first);
                    if(ImGui::Selectable(buf, m_ReferencedData == &pair)) { m_ReferencedData = &pair; }
                }
            } break;
            }

            ImGui::EndChild();
        }
        ImGui::End();
    }

    if(m_HasSelectedReference) {
        switch(m_ReferenceSelectionType) {
        case SEL_FRAME: {
            if(m_ReferencedFrameType == FRAME_SECTOR) {
                switch(m_SelectedFrame->GetType()) {
                case FRAME_LIGHT: {
                    I3D_sector* sector = (I3D_sector*)m_ReferencedData;

                    if(sector) { sector->AddLight((I3D_light*)m_SelectedFrame); }
                } break;
                case FRAME_SOUND: {
                    I3D_sector* sector = (I3D_sector*)m_ReferencedData;

                    if(sector) { sector->AddSound((I3D_sound*)m_SelectedFrame); }
                } break;
                }
            } else {
            }
        } break;
        case SEL_CROSSPOINT: {
            switch(m_WaypointSelectionType) {
            case WPS_NEXTCP: {
                std::pair<uint16_t, RoadCrosspoint>* pair = (std::pair<uint16_t, RoadCrosspoint>*)m_ReferencedData;

                m_SelectedWaypoint->nextCrosspoint = pair->first;
            } break;
            case WPS_PREVCP: {
                std::pair<uint16_t, RoadCrosspoint>* pair = (std::pair<uint16_t, RoadCrosspoint>*)m_ReferencedData;

                m_SelectedWaypoint->prevCrosspoint = pair->first;
            } break;
            case WPS_CPLINK: {
                std::pair<uint16_t, RoadCrosspoint>* pair = (std::pair<uint16_t, RoadCrosspoint>*)m_ReferencedData;

                m_SelectedCrosspoint->directionLinks[m_ReferencedSlotID].crosspointLink = pair->first;
            } break;
            }
        } break;
        case SEL_WAYPOINT: {
            switch(m_WaypointSelectionType) {
            case WPS_NEXTWP: {
                if(m_SelectingCrosspoint) {
                    std::pair<uint16_t, RoadCrosspoint>* pair = (std::pair<uint16_t, RoadCrosspoint>*)m_ReferencedData;

                    m_SelectedWaypoint->nextWaypoint = pair->first;
                } else {
                    std::pair<uint16_t, RoadWaypoint>* pair = (std::pair<uint16_t, RoadWaypoint>*)m_ReferencedData;

                    m_SelectedWaypoint->nextWaypoint = pair->first | 0x8000;
                }
            } break;
            case WPS_PREVWP: {
                if(m_SelectingCrosspoint) {
                    std::pair<uint16_t, RoadCrosspoint>* pair = (std::pair<uint16_t, RoadCrosspoint>*)m_ReferencedData;

                    m_SelectedWaypoint->prevWaypoint = pair->first;
                } else {
                    std::pair<uint16_t, RoadWaypoint>* pair = (std::pair<uint16_t, RoadWaypoint>*)m_ReferencedData;

                    m_SelectedWaypoint->prevWaypoint = pair->first | 0x8000;
                }
            } break;
            case WPS_WPLINK: {
                std::pair<uint16_t, RoadWaypoint>* pair = (std::pair<uint16_t, RoadWaypoint>*)m_ReferencedData;

                m_SelectedWaypoint->nextWaypoint = pair->first | 0x8000;
            } break;
            }
        } break;
        case SEL_NODE: {
            std::pair<uint32_t, WebNode>* pair = (std::pair<uint32_t, WebNode>*)m_ReferencedData;

            m_SelectedNode->links[m_ReferencedSlotID].endPoint = pair->first;
        } break;
        }

        m_HasSelectedReference = false;
    }

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
            ImGui::SetNextWindowBgAlpha(((m_PopupMessageDuration - m_PopupMessageFadeOutStart) - (m_PopupMessageTime - m_PopupMessageFadeOutStart)) /
                                        (m_PopupMessageDuration - m_PopupMessageFadeOutStart));

            auto& style = ImGui::GetStyle();

            ImGui::PushStyleColor(
                ImGuiCol_Border,
                {style.Colors[ImGuiCol_Border].x,
                 style.Colors[ImGuiCol_Border].y,
                 style.Colors[ImGuiCol_Border].z,
                 style.Colors[ImGuiCol_Border].w * ((m_PopupMessageDuration - m_PopupMessageFadeOutStart) - (m_PopupMessageTime - m_PopupMessageFadeOutStart)) /
                     (m_PopupMessageDuration - m_PopupMessageFadeOutStart)});
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);

        if(ImGui::Begin("NotificationOverlay",
                        NULL,
                        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            if(m_PopupMessageTime >= m_PopupMessageFadeOutStart) {
                ImGui::TextColored({1.0f,
                                    1.0f,
                                    1.0f,
                                    ((m_PopupMessageDuration - m_PopupMessageFadeOutStart) - (m_PopupMessageTime - m_PopupMessageFadeOutStart)) /
                                        (m_PopupMessageDuration - m_PopupMessageFadeOutStart)},
                                   "%s",
                                   m_PopupMessage.c_str());
            } else {
                ImGui::Text("%s", m_PopupMessage.c_str());
            }
            ImGui::PopFont();

            ImGui::End();
        }

        ImGui::PopStyleVar();

        if(m_PopupMessageTime >= m_PopupMessageFadeOutStart) { ImGui::PopStyleColor(); }
    }

    if(m_ShowingSceneSettings) {
        if(ImGui::Begin("Scene Settings", &m_ShowingSceneSettings)) {
            ImGui::InputText("Signature text", &m_MissionFileSignature);
            ImGui::ColorEdit3("Clear color", &((S_vector*)((int)m_Scene + 544))->x);
            ImGui::DragFloat("Draw distance", (float*)((int)m_Camera + 328), 0.25f, 2.0f, 5000.0f, "%.2f");
            ImGui::DragFloat("Near clipping plane", (float*)((int)m_Scene + 588));
            ImGui::DragFloat("Far clipping plane", (float*)((int)m_Scene + 592));
            ImGui::End();
        }
    }

    if(m_ShowingCollisionSettings) {
        if(ImGui::Begin("Collision Settings", &m_ShowingCollisionSettings)) {
            ImGui::InputFloat2("Grid cell size", &m_ColManager.grid.cellSize.x, "%.2f");
            ImGui::End();
        }
    }

    if(m_ShowLightmapDialog) {
        static bool bakeBitmap = true;
        static bool coloredEdgeLines = false;
        static bool includeAllLights = true;
        static bool recomputeLightmaps = false;
        static float lmSize = 256.0f;

        if(ImGui::Begin("Lightmap", &m_ShowLightmapDialog)) {
            if(!m_SelectedFrame) {
                if(m_LightmapObjects.size() > 0) {
                    ImGui::Checkbox("Bitmap", &bakeBitmap);
                    ImGui::InputFloat("Scale", &lmSize, 1.0f, 32.0f, "%.1f");
                    ImGui::Checkbox("Colored edge lines", &coloredEdgeLines);
                    ImGui::Checkbox("Include all lights", &includeAllLights);
                    ImGui::Checkbox("Rebuild LM", &recomputeLightmaps);

                    if(ImGui::Button("Bake All LMs")) {
                        for(I3D_lit_object* obj: m_LightmapObjects) {
                            S_vector pos(0, 0, 0);

                            uint32_t flags = 0;

                            if(bakeBitmap) { flags |= LM_BITMAP; }
                            if(includeAllLights) { flags |= LM_INCLUDELIGHTS; }
                            if(coloredEdgeLines) { flags |= LM_DEBUGLINES; }
                            if(recomputeLightmaps) { flags |= LM_BUILD; }

                            obj->Construct(m_Scene, pos, lmSize, flags, NULL, NULL);
                        }
                    }
                } else {
                    ImGui::Text("Before doing any LM operations, select the appropriate frame.");
                }
            } else {
                I3D_FRAME_TYPE type = m_SelectedFrame->GetType();

                if(type != FRAME_VISUAL && type != FRAME_MODEL) {
                    ImGui::Text("Invalid frame type, we can bake lightmaps on models or visuals!");
                } else {
                    if(type == FRAME_VISUAL) {
                        I3D_visual* visual = (I3D_visual*)m_SelectedFrame;
                        I3D_VISUAL_TYPE vType = visual->GetVisualType();

                        if(vType != VISUAL_LIT_OBJECT) {
                            if(vType == VISUAL_OBJECT) {
                                ImGui::Text("In order to do lightmap operations, this object must be converted to lit object.");
                                if(ImGui::Button("Convert")) {
                                    bool ok = false;
                                    I3D_visual* newVisual = m_3DDriver->CreateVisual(VISUAL_LIT_OBJECT);
                                    if(newVisual) {
                                        LS3D_RESULT res = newVisual->Duplicate(visual);
                                        if(I3D_SUCCESS(res)) {
                                            newVisual->LinkTo(visual->GetParent());
                                            while(visual->NumChildren()) {
                                                visual->GetChild(0)->LinkTo(newVisual);
                                            }
                                            newVisual->Update();
                                            visual->LinkTo(nullptr);
                                            visual->Update();
                                            I3D_frame* modelRoot = nullptr;
                                            for(modelRoot = newVisual; (modelRoot = modelRoot->GetParent(), modelRoot) && modelRoot->GetType() != FRAME_MODEL;)
                                                ;
                                            if(modelRoot) {
                                                I3D_model* model = (I3D_model*)modelRoot;

                                                model->DeleteFrame(visual);
                                                model->AddFrame(newVisual);
                                            }

                                            //frame->Release();

                                            m_SelectedFrame = newVisual;
                                            ok = true;
                                        }
                                    }
                                }
                            } else {
                                ImGui::Text("Lightmapping cannot be done on this visual type!");
                            }
                        } else {
                            ImGui::Checkbox("Bitmap", &bakeBitmap);
                            ImGui::InputFloat("Scale", &lmSize, 1.0f, 32.0f, "%.1f");
                            ImGui::Checkbox("Colored edge lines", &coloredEdgeLines);
                            ImGui::Checkbox("Include all lights", &includeAllLights);
                            ImGui::Checkbox("Rebuild LM", &recomputeLightmaps);

                            if(ImGui::Button("Bake All LMs")) {
                                for(I3D_lit_object* obj: m_LightmapObjects) {
                                    S_vector pos(0, 0, 0);

                                    uint32_t flags = 0;

                                    if(bakeBitmap) { flags |= LM_BITMAP; }
                                    if(includeAllLights) { flags |= LM_INCLUDELIGHTS; }
                                    if(coloredEdgeLines) { flags |= LM_DEBUGLINES; }
                                    if(recomputeLightmaps) { flags |= LM_BUILD; }

                                    obj->Construct(m_Scene, pos, lmSize, flags, NULL, NULL);
                                }
                            }

                            if(ImGui::Button("Bake LMs")) {
                                I3D_lit_object* obj = (I3D_lit_object*)visual;

                                S_vector pos(0, 0, 0);

                                uint32_t flags = 0;

                                if(coloredEdgeLines) { flags |= 0x20; }

                                obj->Construct(m_Scene, pos, lmSize, flags, NULL, NULL);
                            }
                        }
                    } else if(type == FRAME_MODEL) {
                    }
                }
            }
            ImGui::End();
        }
    }

    if(m_ShowTransformOptions) {
        static bool setMode = false;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + (viewport->Size.x / 2), viewport->Pos.y + (g_Editor->GetSettings()->video.fullscreen ? 72.0f : 24.0f)),
                                0,
                                {0.5f, 0.5f});

        if(ImGui::Begin("#TransformOptions",
                        nullptr,
                        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
            auto& io = ImGui::GetIO();
            ImGui::PushFont(io.Fonts->Fonts[2]);

            setMode = false;

            ImGui::Checkbox(ICON_FA_BORDER_ALL, &m_ShowTransformGrid);
            if(ImGui::IsItemHovered() && io.KeyShift) {
                ImGui::BeginTooltip();
                ImGui::PushFont(io.Fonts->Fonts[0]);
                ImGui::Text("Toggle grid visibility");
                ImGui::PopFont();
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            ImGui::Checkbox(ICON_FA_MAGNET, &m_TransformGridSnapping);
            if(ImGui::IsItemHovered() && io.KeyShift) {
                ImGui::BeginTooltip();
                ImGui::PushFont(io.Fonts->Fonts[0]);
                ImGui::Text("Toggle grid snapping");
                ImGui::PopFont();
                ImGui::EndTooltip();
            }
            ImGui::SameLine();

            ImGui::Dummy(ImVec2(8.0f, 0.0f));

            ImGui::SameLine();

            if(m_CurrentTransformOperation == ImGuizmo::TRANSLATE) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Tab));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_TabHovered));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
            }
            if(ImGui::Button(ICON_FA_UP_DOWN_LEFT_RIGHT)) {
                m_CurrentTransformOperation = ImGuizmo::TRANSLATE;
                setMode = true;
            }
            if(ImGui::IsItemHovered() && io.KeyShift) {
                ImGui::BeginTooltip();
                ImGui::PushFont(io.Fonts->Fonts[0]);
                ImGui::Text("Translate");
                ImGui::PopFont();
                ImGui::EndTooltip();
            }
            if(m_CurrentTransformOperation == ImGuizmo::TRANSLATE && !setMode) { ImGui::PopStyleColor(3); }
            ImGui::SameLine();
            if(m_CurrentTransformOperation == ImGuizmo::ROTATE) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Tab));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_TabHovered));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
            }
            if(ImGui::Button(ICON_FA_ARROWS_ROTATE)) {
                m_CurrentTransformOperation = ImGuizmo::ROTATE;
                setMode = true;
            }
            if(ImGui::IsItemHovered() && io.KeyShift) {
                ImGui::BeginTooltip();
                ImGui::PushFont(io.Fonts->Fonts[0]);
                ImGui::Text("Rotate");
                ImGui::PopFont();
                ImGui::EndTooltip();
            }
            if(m_CurrentTransformOperation == ImGuizmo::ROTATE && !setMode) { ImGui::PopStyleColor(3); }
            ImGui::SameLine();
            if(m_CurrentTransformOperation == ImGuizmo::SCALE) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Tab));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_TabHovered));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
            }
            if(ImGui::Button(ICON_FA_EXPAND)) {
                m_CurrentTransformOperation = ImGuizmo::SCALE;
                setMode = true;
            }
            if(ImGui::IsItemHovered() && io.KeyShift) {
                ImGui::BeginTooltip();
                ImGui::PushFont(io.Fonts->Fonts[0]);
                ImGui::Text("Scale");
                ImGui::PopFont();
                ImGui::EndTooltip();
            }
            if(m_CurrentTransformOperation == ImGuizmo::SCALE && !setMode) { ImGui::PopStyleColor(3); }
            ImGui::SameLine();

            ImGui::Dummy(ImVec2(8.0f, 0.0f));
            ImGui::SameLine();

            ImGui::PopFont();
            if(ImGui::InputFloat("Grid scale", &m_GridScale, 0.1f, 1.0f, "%.3f")) {
                if(m_GridScale < 0) { m_GridScale = 0; }
            }
            ImGui::SameLine();
            if(ImGui::InputInt("Grid size", &m_GridSize, 1, 4)) {
                if(m_GridSize < 0) { m_GridSize = 0; }
            }

            ImGui::End();
        }
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX8_RenderDrawData(ImGui::GetDrawData());

    QueryPerformanceCounter((LARGE_INTEGER*)&g_FrameEnd);
    g_ElapsedTime = ElapsedMicroseconds(g_FrameStart, g_FrameEnd);

    while(g_ElapsedTime < TARGET_FRAME_TIME) {
        if((g_ElapsedTime + g_OverSleepDuration) >= TARGET_FRAME_TIME) {
            g_OverSleepDuration -= TARGET_FRAME_TIME - g_ElapsedTime;
            break;
        }

        Sleep(1);

        QueryPerformanceCounter((LARGE_INTEGER*)&g_FrameEnd);
        g_ElapsedTime = ElapsedMicroseconds(g_FrameStart, g_FrameEnd);

        if(g_ElapsedTime > TARGET_FRAME_TIME) g_OverSleepDuration += g_ElapsedTime - TARGET_FRAME_TIME;
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&g_FrameEnd);
    g_TicksAccum += g_FrameEnd - g_FrameStart;
    g_FrameCount += 1;

    if((g_FrameCount % TARGET_FPS_CAP) == 0) {
        g_AverageFPS = ((g_QPCFreq * TARGET_FPS_CAP) + (g_TicksAccum - 1)) / g_TicksAccum; // round-off
        m_DeltaTime = 1.0f / g_AverageFPS;
        g_TicksAccum = 0;
        g_FrameCount = 0;
    }

    //m_DeltaTime = float(ElapsedMicroseconds(g_FrameStart, g_FrameEnd)) / 1000000.0f;

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

void SceneEditor::Shutdown() {
    Clear();

    ImGui::SetCurrentContext(m_ImGuiContext);

    ImGui_ImplWin32_Shutdown();
    ImGui_ImplDX8_Shutdown();
    //ImGui_ImplDInput_Shutdown();

    ImGui::DestroyContext(m_ImGuiContext);

    m_SoundDriver->Close();
    m_3DDriver->Close();
    m_IGraph->Close();

    DestroyMenu(m_MenuBar);
    m_MenuBar = nullptr;
}

Actor* SceneEditor::GetActor(I3D_frame* frame) {
    for(auto actor: m_Actors) {
        if(actor->GetFrame() == frame) return actor;
    }

    return nullptr;
}

Actor* SceneEditor::CreateActor(ActorType type, I3D_frame* frame) {
    if(GetActor(frame)) { return nullptr; }

    Actor* actor = nullptr;

    switch(type) {
    case ACTOR_PLAYER: {
        if(frame->GetType() != FRAME_MODEL) {
            ShowPopupMessage("Player actor can be created only on models!");

            return nullptr;
        }
        actor = new Player();

        I3D_animation_set* anim = m_3DDriver->CreateAnimationSet();

        anim->Open("Anims\\breath01a.i3d");

        I3D_model* model = (I3D_model*)frame;

        model->SetAnimation(anim, 0, 1.0f, 0);
        model->SetAnimTime(0, 0);
        model->StartAnimation(0);

        m_AnimatedModels.push_back(model);
    } break;
    case ACTOR_CAR: {
        if(frame->GetType() != FRAME_MODEL) {
            ShowPopupMessage("Car actor can be created only on models!");

            return nullptr;
        }
        actor = new Car();
    } break;
    case ACTOR_DETECTOR: actor = new Detector(); break;
    case ACTOR_DOOR: {
        if(frame->GetType() != FRAME_MODEL && frame->GetType() != FRAME_VISUAL) {
            ShowPopupMessage("Door actor can be created only on models or visuals!");

            return nullptr;
        }

        actor = new Door();
    } break;
    case ACTOR_RAILWAY: {
        if(frame->GetType() != FRAME_MODEL) {
            ShowPopupMessage("Railway actor can be created only on models!");

            return nullptr;
        }
        actor = new Railway();
    } break;
    case ACTOR_TRAFFIC: actor = new TrafficManager(); break;
    case ACTOR_PEDESTRIANS: actor = new PedManager(); break;
    case ACTOR_ENEMY: {
        if(frame->GetType() != FRAME_MODEL) {
            ShowPopupMessage("Human actor can be created only on models!");

            return nullptr;
        }
        actor = new Enemy();

        I3D_animation_set* anim = m_3DDriver->CreateAnimationSet();

        anim->Open("Anims\\breath01a.i3d");

        I3D_model* model = (I3D_model*)frame;

        model->SetAnimation(anim, 0, 1.0f, 0);
        model->SetAnimTime(0, 0);
        model->StartAnimation(0);

        m_AnimatedModels.push_back(model);
    } break;
    case ACTOR_PHYSICAL: {
        if(frame->GetType() != FRAME_MODEL) {
            ShowPopupMessage("Physical actor can be created only on models!");

            return nullptr;
        }
        actor = new Physical();
    } break;
    }

    if(actor) {
        actor->SetFrame(frame);

        m_Actors.push_back(actor);
    }

    return actor;
}

void SceneEditor::DeleteActor(I3D_frame* frame) {
    if(!GetActor(frame)) return;

    int i = 0;

    for(auto actor: m_Actors) {
        if(actor->GetFrame() == frame) break;

        i++;
    }

    auto it = std::find(m_AnimatedModels.begin(), m_AnimatedModels.end(), frame);
    if(it != m_AnimatedModels.end()) {
        I3D_model* model = (I3D_model*)frame;
        model->StopAnimation(0);
        I3D_animation_set* anim = model->GetAnimationSet(0);
        model->SetAnimation(NULL, 0, 1.0F, 0);
        model->ResetAllPoses();
        anim->Release();

        m_AnimatedModels.erase(it);
    }

    if(m_SelectedActor == m_Actors[i]) m_SelectedActor = nullptr;

    delete m_Actors[i];
    m_Actors.erase(m_Actors.begin() + i);
}

I3D_dummy* SceneEditor::CreateDummy(bool inFrontOfCamera) {
    I3D_dummy* dummy = (I3D_dummy*)m_3DDriver->CreateFrame(FRAME_DUMMY);

    dummy->SetName("New Dummy");
    dummy->SetPos(m_CameraPos + DirFromEuler({m_CameraRot.x, -m_CameraRot.y, 0}) * 6.5f);
    dummy->SetScale({1, 1, 1});

    m_Scene->AddFrame(dummy);
    dummy->LinkTo(m_PrimarySector);

    m_Frames.push_back(dummy);
    m_CreatedFrames.push_back(dummy);

    auto psEntry = FindEntryByName("Primary sector");

    if(psEntry) {
        HierarchyEntry entry;
        entry.frame = dummy;
        entry.name = "New Dummy";
        entry.parent = psEntry;

        psEntry->children.push_back(entry);
    }

    SelectFrame(dummy);

    return dummy;
}

I3D_sound* SceneEditor::CreateSound(const std::string& soundName, bool inFrontOfCamera) {
    I3D_sound* sound = (I3D_sound*)m_3DDriver->CreateFrame(FRAME_SOUND);

    if(sound->Open(("Sounds\\" + soundName).c_str(), 0, nullptr, nullptr) == I3D_OK) { g_SoundsMap[sound] = soundName; }

    sound->SetName("New Sound");
    sound->SetPos(m_CameraPos + DirFromEuler({m_CameraRot.x, -m_CameraRot.y, 0}) * 6.5f);
    sound->SetScale({1, 1, 1});

    m_Scene->AddFrame(sound);
    sound->LinkTo(m_PrimarySector);

    m_Frames.push_back(sound);
    m_CreatedFrames.push_back(sound);

    auto psEntry = FindEntryByName("Primary sector");

    if(psEntry) {
        HierarchyEntry entry;
        entry.frame = sound;
        entry.name = "New Sound";
        entry.parent = psEntry;

        psEntry->children.push_back(entry);
    }

    SelectFrame(sound);

    return sound;
}

I3D_light* SceneEditor::CreateLight(bool inFrontOfCamera) {
    I3D_light* light = (I3D_light*)m_3DDriver->CreateFrame(FRAME_LIGHT);

    light->SetName("New Light");
    light->SetPos(m_CameraPos + DirFromEuler({m_CameraRot.x, -m_CameraRot.y, 0}) * 6.5f);
    light->SetScale({1, 1, 1});

    m_Scene->AddFrame(light);
    light->LinkTo(m_PrimarySector);
    m_PrimarySector->AddLight(light);

    m_Frames.push_back(light);
    m_CreatedFrames.push_back(light);

    auto psEntry = FindEntryByName("Primary sector");

    if(psEntry) {
        HierarchyEntry entry;
        entry.frame = light;
        entry.name = "New Light";
        entry.parent = psEntry;

        psEntry->children.push_back(entry);
    }

    SelectFrame(light);

    return light;
}

I3D_model* SceneEditor::CreateModel(const std::string& modelName, bool inFrontOfCamera) {
    I3D_model* model = (I3D_model*)m_3DDriver->CreateFrame(FRAME_MODEL);
    if(model->Open(("Models\\" + modelName).c_str()) == I3D_OK) { g_ModelsMap[model] = modelName; }

    model->SetName("New Model");
    model->SetPos(m_CameraPos + DirFromEuler({m_CameraRot.x, -m_CameraRot.y, 0}) * 6.5f);
    model->SetScale({1, 1, 1});

    m_Scene->AddFrame(model);
    model->LinkTo(m_PrimarySector);

    m_Frames.push_back(model);
    m_CreatedFrames.push_back(model);

    auto psEntry = FindEntryByName("Primary sector");

    if(psEntry) {
        HierarchyEntry entry;
        entry.frame = model;
        entry.name = "New Model";
        entry.parent = psEntry;

        psEntry->children.push_back(entry);
    }

    SelectFrame(model);

    return model;
}

SceneEditor::RoadCrosspoint* SceneEditor::CreateCrosspoint() {
    m_HighestCrosspointID++;

    m_RoadCrosspoints.insert({m_HighestCrosspointID, {}});

    return &m_RoadCrosspoints[m_HighestCrosspointID];
}

SceneEditor::RoadWaypoint* SceneEditor::CreateWaypoint() {
    m_HighestWaypointID++;

    m_RoadWaypoints.insert({m_HighestWaypointID, {}});

    return &m_RoadWaypoints[m_HighestWaypointID];
}

SceneEditor::WebNode* SceneEditor::CreateWebNode() {
    m_HighestNodeID++;

    m_WebNodes.insert({m_HighestNodeID, {}});

    return &m_WebNodes[m_HighestNodeID];
}

GameScript* SceneEditor::CreateScript() {
    GameScript& script = m_Scripts.emplace_back();
    script.frame = nullptr;
    script.name = "New Script";
    script.script = "";

    return &script;
}

void SceneEditor::PasteFrame() {
    if(!m_CopiedObject && m_CopySelectionType != SEL_FRAME) return;

    I3D_frame* copiedFrame = (I3D_frame*)m_CopiedObject;
    I3D_frame* newFrame = nullptr;

    if(copiedFrame->GetType() != FRAME_VISUAL) {
        newFrame = m_3DDriver->CreateFrame(copiedFrame->GetType());
    } else {
        I3D_visual* visual = (I3D_visual*)copiedFrame;
        newFrame = m_3DDriver->CreateVisual(visual->GetVisualType());
    }

    if(newFrame) {
        std::string name = std::string(copiedFrame->GetName()) + " - Copy";
        newFrame->Duplicate(copiedFrame);
        newFrame->SetName(name.c_str());
        newFrame->SetPos(m_CameraPos + DirFromEuler({m_CameraRot.x, -m_CameraRot.y, 0}) * 6.5f);
        newFrame->SetRot(copiedFrame->GetRot());
        newFrame->SetScale(copiedFrame->GetScale());
        newFrame->LinkTo(copiedFrame->GetParent());
        newFrame->Update();

        if(copiedFrame->GetType() == FRAME_MODEL) {
            g_ModelsMap[(I3D_model*)newFrame] = g_ModelsMap[(I3D_model*)copiedFrame];
        } else if(copiedFrame->GetType() == FRAME_SOUND) {
            g_SoundsMap[(I3D_sound*)newFrame] = g_SoundsMap[(I3D_sound*)copiedFrame];
        }

        Actor* copiedActor = GetActor(copiedFrame);

        if(copiedActor) {
            Actor* actor = CreateActor(copiedActor->GetType(), newFrame);
            actor->Duplicate(copiedActor);
            //actor->SetName(newFrame->GetName());
        }

        SelectFrame(newFrame);

        copiedFrame = nullptr;

        m_Scene->AddFrame(newFrame);
        m_Frames.push_back(newFrame);
    }
}

void SceneEditor::CopySelection() {
    if(m_SelectedFrame != nullptr) {
        CopyFrame(m_SelectedFrame);
    } else if(m_SelectedCrosspoint != nullptr) {
        m_CopySelectionType = SEL_CROSSPOINT;
        m_CopiedObject = m_SelectedCrosspoint;
    } else if(m_SelectedWaypoint != nullptr) {
        m_CopySelectionType = SEL_WAYPOINT;
        m_CopiedObject = m_SelectedWaypoint;
    } else if(m_SelectedNode != nullptr) {
        m_CopySelectionType = SEL_NODE;
        m_CopiedObject = m_SelectedNode;
    } else if(m_SelectedScript != nullptr) {
        m_CopySelectionType = SEL_SCRIPT;
        m_CopiedObject = m_SelectedScript;
    } else {
        ShowPopupMessage("There's nothing to copy!");
    }
}

void SceneEditor::PasteSelection() {
    switch(m_CopySelectionType) {
    case SEL_FRAME: PasteFrame(); break;
    case SEL_CROSSPOINT: {
        RoadCrosspoint* crossToCopy = (RoadCrosspoint*)m_CopiedObject;
        RoadCrosspoint* cross = CreateCrosspoint();
        cross->pos = crossToCopy->pos;
        cross->speed = crossToCopy->speed;
        cross->hasSemaphore = crossToCopy->hasSemaphore;
    } break;
    case SEL_WAYPOINT: {
        RoadWaypoint* pointToCopy = (RoadWaypoint*)m_CopiedObject;
        RoadWaypoint* point = CreateWaypoint();
        point->pos = pointToCopy->pos;
        point->speed = pointToCopy->speed;
    } break;
    case SEL_NODE: {
        WebNode* nodeToCopy = (WebNode*)m_CopiedObject;
        WebNode* node = CreateWebNode();
        node->pos = nodeToCopy->pos;
        node->type = nodeToCopy->type;
        node->id = nodeToCopy->id;
        node->radius = nodeToCopy->radius;
    } break;
    case SEL_SCRIPT: {
        if(m_CopiedObject == m_SelectedScript) {
            ShowPopupMessage("You must select a different script to paste the copied script!");
        } else {
            GameScript* script = (GameScript*)m_CopiedObject;
            m_SelectedScript->script = script->script;
        }
    } break;
    default: ShowPopupMessage("There's nothing to paste!"); break;
    }
}

void SceneEditor::DeleteSelection() {
    if(m_SelectedFrame != nullptr) {
        DeleteFrame(m_SelectedFrame);
    } else if(m_SelectedCrosspoint != nullptr) {
        if(m_RoadCrosspoints.contains(m_SelectedCrosspointID)) {
            // Clear references to this crosspoint
            for(int i = 0; i < 4; i++) {
                RoadWaypoint* point = nullptr;
                if(m_SelectedCrosspoint->waypointLinks[i] != UINT16_MAX) { point = &m_RoadWaypoints[m_SelectedCrosspoint->waypointLinks[i] & 0x7FFF]; }
                if(point) {
                    if(point->prevCrosspoint == m_SelectedCrosspointID) point->prevCrosspoint = UINT16_MAX;
                    if(point->nextCrosspoint == m_SelectedCrosspointID) point->nextCrosspoint = UINT16_MAX;

                    if(point->prevWaypoint == m_SelectedCrosspointID) point->prevWaypoint = UINT16_MAX;
                    if(point->nextWaypoint == m_SelectedCrosspointID) point->nextWaypoint = UINT16_MAX;
                }

                RoadCrosspoint* cross = nullptr;
                if(m_SelectedCrosspoint->directionLinks[i].crosspointLink != UINT16_MAX) {
                    cross = &m_RoadCrosspoints[m_SelectedCrosspoint->directionLinks[i].crosspointLink];
                }
                if(cross) {
                    for(int j = 0; j < 4; j++) {
                        if(cross->directionLinks[j].crosspointLink == m_SelectedCrosspointID) { cross->directionLinks[j].crosspointLink = UINT16_MAX; }
                    }
                }
            }

            m_RoadCrosspoints.erase(m_SelectedCrosspointID);
            ClearSelection();
        }
    } else if(m_SelectedWaypoint != nullptr) {
        if(m_RoadWaypoints.contains(m_SelectedWaypointID)) {
            // Clear references to this waypoint
            RoadWaypoint* point = nullptr;
            RoadCrosspoint* cross = nullptr;

            if(m_SelectedWaypoint->nextWaypoint != UINT16_MAX) {
                if(m_SelectedWaypoint->nextWaypoint & 0x8000) {
                    point = &m_RoadWaypoints[m_SelectedWaypoint->nextWaypoint & 0x7FFF];
                } else {
                    cross = &m_RoadCrosspoints[m_SelectedWaypoint->nextWaypoint];
                }
            }
            if(point) {
                if((point->nextWaypoint & 0x7FFF) == m_SelectedWaypointID) point->nextWaypoint = UINT16_MAX;
                if((point->prevWaypoint & 0x7FFF) == m_SelectedWaypointID) point->prevWaypoint = UINT16_MAX;
            } else if(cross) {
                for(int i = 0; i < 4; i++) {
                    if((cross->waypointLinks[i] & 0x7FFF) == m_SelectedWaypointID) { cross->waypointLinks[i] = UINT16_MAX; }
                }
            }

            if(m_SelectedWaypoint->prevWaypoint != UINT16_MAX) {
                if(m_SelectedWaypoint->prevWaypoint & 0x8000) {
                    point = &m_RoadWaypoints[m_SelectedWaypoint->prevWaypoint & 0x7FFF];
                } else {
                    cross = &m_RoadCrosspoints[m_SelectedWaypoint->prevWaypoint];
                }
            }
            if(point) {
                if((point->nextWaypoint & 0x7FFF) == m_SelectedWaypointID) point->nextWaypoint = UINT16_MAX;
                if((point->prevWaypoint & 0x7FFF) == m_SelectedWaypointID) point->prevWaypoint = UINT16_MAX;
            } else if(cross) {
                for(int i = 0; i < 4; i++) {
                    if((cross->waypointLinks[i] & 0x7FFF) == m_SelectedWaypointID) { cross->waypointLinks[i] = UINT16_MAX; }
                }
            }

            m_RoadWaypoints.erase(m_SelectedWaypointID);
            ClearSelection();
        }
    } else if(m_SelectedNode != nullptr) {
        if(m_WebNodes.contains(m_SelectedNodeID)) {
            // Update webnode IDs and references + remove unused links
            for(auto& pair: m_WebNodes) {
                auto& node = pair.second;
                //if(node.id > index) { node.id--; }
                int i = 0;
                std::vector<int> toRemove;
                for(auto& link: node.links) {
                    if(link.endPoint > m_SelectedNodeID) { link.endPoint--; }

                    if(link.endPoint == m_SelectedNodeID) { toRemove.push_back(i); }

                    i++;
                }

                for(int indexToRemove: toRemove) {
                    node.links.erase(node.links.begin() + indexToRemove);
                }
            }

            m_WebNodes.erase(m_SelectedNodeID);
            ClearSelection();
        }
    } else if(m_SelectedScript != nullptr) {
        auto it = std::find_if(m_Scripts.begin(), m_Scripts.end(), [this](const auto& script) { return &script == m_SelectedScript; });
        if(it != m_Scripts.end()) {
            m_Scripts.erase(it);
            ClearSelection();
        }
    } else {
        ShowPopupMessage("No object is selected!");
    }
}

void SceneEditor::DeleteFrame(I3D_frame* frame) {
    auto it = std::find(m_CreatedFrames.begin(), m_CreatedFrames.end(), frame);
    auto it2 = std::find(m_Frames.begin(), m_Frames.end(), frame);
    if(frame && it != m_CreatedFrames.end()) {
        m_Scene->DeleteFrame(frame);
        m_CreatedFrames.erase(it);
        m_Frames.erase(it2);
        Actor* actor = GetActor(frame);
        if(actor) { DeleteActor(frame); }
        frame->Release();
        if(m_SelectedFrame == frame) { ClearSelection(); }
        if(m_CopiedObject == frame) {
            m_CopySelectionType = SEL_NONE;
            m_CopiedObject = nullptr;
        }
    } else if(frame && it2 != m_Frames.end()) {
        m_Scene->DeleteFrame(frame);
        m_CreatedFrames.erase(it);
        m_Frames.erase(it2);
        Actor* actor = GetActor(frame);
        if(actor) { DeleteActor(frame); }
        frame->Release();
        if(m_SelectedFrame == frame) { ClearSelection(); }
        if(m_CopiedObject == frame) {
            m_CopySelectionType = SEL_NONE;
            m_CopiedObject = nullptr;
        }
    } else {
        ShowPopupMessage("Cannot delete a non-deletable frame!");
    }
}

I3DENUMRET __stdcall EnumColliders(I3D_frame* frame, uint32_t user) {
    SceneEditor* editor = (SceneEditor*)user;

    if(strlen(frame->GetName()) >= 6 && !strnicmp(frame->GetName(), "LLwcol", 6)) { 
        frame->LinkTo(editor->GetScene()->GetPrimarySector(), 1);
        frame->SetOn(false);
    }

    if(strlen(frame->GetName()) >= 4 && !strnicmp(frame->GetName(), "wcol", 4)) {
        frame->LinkTo(editor->GetScene()->GetPrimarySector(), 1);
        frame->SetOn(false);
    }

    return I3DENUMRET_OK;
}

bool SceneEditor::Load(const std::string& path) {
    Clear();

    m_Scene = (I3D_scene*)m_3DDriver->CreateFrame(FRAME_SCENE);
    m_PrimarySector = m_Scene->GetPrimarySector();
    m_BackdropSector = m_Scene->GetBackdropSector();

    m_Scene->SetActiveCamera(m_Camera);

    m_CameraPos = {0, 0, 0};
    m_CameraRot = {0, 0};

    DrawProgress("Loading: scene.4ds");
    debugPrintf("Loading: scene.4ds");
    LS3D_RESULT res = m_Scene->Open((path + "\\scene.i3d").c_str(), 0, nullptr, nullptr);

    if(I3D_FAIL(res)) { debugPrintf("Failed to load %s!", (path + "\\scene.4ds").c_str()); }

    DrawProgress("Loading: scene2.bin");
    debugPrintf("Loading: scene2.bin");
    LoadSceneBin(path + "\\scene2.bin");

    DrawProgress("Loading: Building scene tree");
    debugPrintf("Building scene tree...");
    m_Hierarchy = BuildSceneTree();

    m_Scene->EnumFrames(EnumColliders, (uint32_t)this, ENUMF_VISUAL, 0);

    DrawProgress("Loading: cache.bin");
    debugPrintf("Loading: cache.bin");
    LoadCacheBin(path + "\\cache.bin");

    DrawProgress("Loading: tree.klz");
    debugPrintf("Loading: tree.klz");
    LoadTreeKlz(path + "\\tree.klz");

    DrawProgress("Loading: road.bin");
    debugPrintf("Loading: road.bin");
    LoadRoadBin(path + "\\road.bin");

    DrawProgress("Loading: check.bin");
    debugPrintf("Loading: check.bin");
    LoadCheckBin(path + "\\check.bin");

    m_MissionPath = path;
    EnableMenuItem(m_FileMenu, MCMD_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED);

    char buf[256];
    sprintf(buf, "Scene Editor - %s", path.c_str());

    m_SoundDriver->SetVolume(0.25f);

    m_IGraph->SetAppName(buf);

    m_SceneLoaded = true;
    g_ChangesMade = false;

    return true;
}

bool SceneEditor::LoadSceneBin(const std::string& fileName) {
    BinaryReader reader(fileName);
    if(!reader.IsOpen()) {
        debugPrintf("Failed to load %s!", fileName.c_str());
        return false;
    }

    ChunkReader chunk(&reader);
    if(++chunk == CT_BASECHUNK) {
        while(chunk) {
            ChunkType type = ++chunk;
            if(!ReadChunk((E_CHUNK_TYPE)type, chunk)) { break; }
            --chunk;
        }
        --chunk;
    }

    return true;
}

bool SceneEditor::LoadCacheBin(const std::string& fileName) {
    BinaryReader reader(fileName);
    if(reader.IsOpen()) {
        ChunkReader chunk(&reader);
        if(CT_CITY == ++chunk) {
            auto version = chunk.Read<uint32_t>();
            while(chunk) {
                if(CT_PART == ++chunk) {
                    CityPart& part = m_CacheParts.emplace_back();

                    part.name = chunk.ReadString();

                    I3D_frame* baseFrame = m_Scene->FindFrame(part.name.c_str(), ENUMF_ALL);

                    if(baseFrame) {
                        part.frame = baseFrame;
                    } else {
                        part.frame = nullptr;
                    }
                    part.spherePos = chunk.Read<S_vector2>();
                    part.sphereRadius = chunk.Read<float>();
                    part.leftBottomCorner = chunk.Read<S_vector2>();
                    part.leftSide = chunk.Read<S_vector2>();
                    part.rightBottomCorner = chunk.Read<S_vector2>();
                    part.rightSide = chunk.Read<S_vector2>();
                    part.leftTopCorner = chunk.Read<S_vector2>();
                    part.bottomSide = chunk.Read<S_vector2>();
                    part.rightTopCorner = chunk.Read<S_vector2>();
                    part.topSide = chunk.Read<S_vector2>();

                    part.bbox.min = {part.leftBottomCorner.x, baseFrame->GetWorldPos().y, part.leftBottomCorner.y};
                    part.bbox.max = {part.rightTopCorner.x, baseFrame->GetWorldPos().y + 64.0f, part.rightTopCorner.y};

                    int i = 0;

                    while(chunk) {
                        if(CT_FRAME == ++chunk) {
                            auto frameModel = chunk.ReadString();
                            auto pos = chunk.Read<S_vector>();
                            auto rot = chunk.Read<S_quat>();
                            auto scale = chunk.Read<S_vector>();
                            auto unk = chunk.Read<uint32_t>();
                            auto scale2 = chunk.Read<S_vector>();

                            //debugPrintf("Creating model (cache): %s", name.c_str());

                            I3D_model* model = Cache::Open(frameModel);

                            if(model) { g_ModelsMap[model] = frameModel; }

                            // Set common frame properties
                            model->SetName((part.name + "_segment" + std::to_string(i)).c_str());

                            model->LinkTo(m_PrimarySector);

                            m_Scene->AddFrame(model);
                            part.models.push_back(model);

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

    return false;
}

bool SceneEditor::LoadRoadBin(const std::string& fileName) {
    std::vector<RoadWaypoint> waypoints;
    std::vector<RoadCrosspoint> crosspoints;

    BinaryReader reader(fileName);
    if(reader.IsOpen()) {
        uint32_t magic = reader.ReadUInt32();

        if(magic != 2) return false;

        uint32_t numCrossroads = reader.ReadUInt32();
        crosspoints.resize(numCrossroads);

        for(uint32_t i = 0; i < numCrossroads; i++) {
            RoadCrosspoint& cross = crosspoints[i];

            cross.pos = reader.ReadVec3();
            cross.hasSemaphore = reader.ReadBoolean();
            reader.Read(cross.pad0, 3);
            cross.speed = SPEED_GAME_TO_KMH(reader.ReadSingle());
            for(int j = 0; j < 4; j++) {
                cross.waypointLinks[j] = reader.ReadUInt16();
            }

            for(int j = 0; j < 4; j++) {
                RoadDirectionLink& link = cross.directionLinks[j];

                link.crosspointLink = reader.ReadUInt16();
                link.unk1 = reader.ReadUInt16();
                link.distance = reader.ReadSingle();
                link.angle = DEG(reader.ReadSingle());
                link.unk2 = reader.ReadUInt16();
                link.priority = reader.ReadUInt32();
                link.unk3 = reader.ReadUInt16();

                for(int k = 0; k < 4; k++) {
                    RoadLane& lane = link.lanes[k];

                    lane.unk1 = reader.ReadUInt16();
                    lane.type = reader.ReadUInt8();
                    lane.unk2 = reader.ReadUInt8();
                    lane.distance = reader.ReadSingle();
                }
            }
        }

        uint32_t numWaypoints = reader.ReadUInt32();
        waypoints.resize(numWaypoints);

        for(uint32_t i = 0; i < numWaypoints; i++) {
            RoadWaypoint& waypoint = waypoints[i];

            waypoint.pos = reader.ReadVec3();
            waypoint.speed = SPEED_GAME_TO_KMH(reader.ReadSingle());
            waypoint.prevWaypoint = reader.ReadUInt16();
            waypoint.nextWaypoint = reader.ReadUInt16();
            waypoint.prevCrosspoint = reader.ReadUInt16();
            waypoint.nextCrosspoint = reader.ReadUInt16();
        }

        uint16_t id = 0;

        for(auto& cp: crosspoints) {
            m_RoadCrosspoints.insert({id, cp});

            id++;
        }
        m_HighestCrosspointID = id;

        id = 0;

        for(auto& wp: waypoints) {
            m_RoadWaypoints.insert({id, wp});

            id++;
        }
        m_HighestWaypointID = id;

        return true;
    }

    return false;
}

bool SceneEditor::LoadCheckBin(const std::string& fileName) {
    std::vector<WebNode> nodes;

    BinaryReader reader(fileName);
    if(reader.IsOpen()) {
        uint32_t magic = reader.ReadUInt32();

        if(magic != 0x1ABCEDF) return false;

        uint32_t numPoints = reader.ReadUInt32();

        nodes.resize(numPoints);

        for(uint32_t i = 0; i < numPoints; i++) {
            WebNode& node = nodes[i];
            node.pos = reader.ReadVec3();
            node.type = (WebNodeType)reader.ReadUInt16();
            node.id = reader.ReadUInt16();
            node.radius = reader.ReadUInt16();
            reader.Read(node.unk, 10);
            node.numEnterLinks = reader.ReadUInt8();
            node.numExitLinks = reader.ReadUInt8();

            node.links.resize(node.numEnterLinks);
        }

        for(uint32_t i = 0; i < numPoints; i++) {
            WebNode& node = nodes[i];

            for(uint8_t i = 0; i < node.numEnterLinks; i++) {
                WebConnection& link = node.links[i];

                link.endPoint = reader.ReadUInt16();
                link.type = (WebConnectionType)reader.ReadUInt16();
                link.length = reader.ReadSingle();
            }
        }

        uint16_t id = 0;

        for(auto& node: nodes) {
            m_WebNodes.insert({id, node});

            id++;
        }
        m_HighestNodeID = id;

        return true;
    }

    return false;
}

bool SceneEditor::LoadTreeKlz(const std::string& fileName) {
    BinaryReader reader(fileName);
    if(!reader.IsOpen()) {
        debugPrintf("Failed to load %s: Failed to open the file", fileName.c_str());
        return false;
    }

    std::string fourCC = reader.ReadFixedString(4);
    if(fourCC != "GifC") {
        debugPrintf("Failed to load %s: Not a valid collision file", fileName.c_str());
        return false;
    }

    uint32_t version = reader.ReadUInt32();
    if(version != 0x00000005) { // VERSION_MAFIA
        debugPrintf("Failed to load %s: Invalid file version", fileName.c_str());
        return false;
    }

    uint32_t gridDataOffset = reader.ReadUInt32();
    uint32_t numLinks = reader.ReadUInt32();
    reader.ReadUInt32();
    reader.ReadUInt32();

    std::vector<uint32_t> linkOffsets(numLinks);
    for(uint32_t& offset: linkOffsets) {
        offset = reader.ReadUInt32();
    }

    m_ColManager.links.resize(numLinks);
    for(size_t i = 0; i < numLinks; ++i) {
        reader.Seek(linkOffsets[i], SEEK_SET);
        m_ColManager.links[i].type = (CollisionLink::LinkType)reader.ReadUInt32();
        std::string name = reader.ReadNullTerminatedString();
        m_ColManager.links[i].frame = FindFrameByName(name);
        if(!m_ColManager.links[i].frame) {
            debugPrintf("The tree.klz file is referencing an non-existent frame \"%s\", all colliders referencing this frame will be ignored.", name.c_str());
        }
        size_t len = name.length() + 1;
        int padding = static_cast<int>(len % 4);
        if(padding != 0) { reader.Seek(4 - padding, SEEK_CUR); }
    }

    reader.Seek(gridDataOffset, SEEK_SET);

    CollisionGrid& grid = m_ColManager.grid;
    grid.min = reader.ReadVec2();
    grid.max = reader.ReadVec2();
    grid.cellSize = reader.ReadVec2();
    grid.width = reader.ReadUInt32();
    grid.length = reader.ReadUInt32();
    reader.ReadSingle();

    reader.Seek(3 * sizeof(int32_t), SEEK_CUR); // Skip 3 int32s for VERSION_MAFIA

    uint32_t numFaces = reader.ReadUInt32();
    reader.Seek(4, SEEK_CUR); // Skip reserved pointer

    uint32_t numXTOBBs = reader.ReadUInt32();
    reader.Seek(4, SEEK_CUR);

    uint32_t numAABBs = reader.ReadUInt32();
    reader.Seek(4, SEEK_CUR);

    uint32_t numSpheres = reader.ReadUInt32();
    reader.Seek(4, SEEK_CUR);

    uint32_t numOBBs = reader.ReadUInt32();
    reader.Seek(4, SEEK_CUR);

    uint32_t numCylinders = reader.ReadUInt32();
    reader.Seek(4, SEEK_CUR);

    reader.Seek(2 * sizeof(int32_t), SEEK_CUR); // Skip 2 int32s for VERSION_MAFIA

    grid.bounds.x.resize(grid.width + 1);
    for(float& val: grid.bounds.x) {
        val = reader.ReadSingle();
    }

    grid.bounds.y.resize(grid.length + 1);
    for(float& val: grid.bounds.y) {
        val = reader.ReadSingle();
    }

    reader.Seek(sizeof(int32_t), SEEK_CUR); // Skip 1 int32 for VERSION_MAFIA

    size_t i = 0;

    std::vector<size_t> trisToRemove;
    std::vector<size_t> aabbsToRemove;
    std::vector<size_t> obbsToRemove;
    std::vector<size_t> xtobbsToRemove;
    std::vector<size_t> spheresToRemove;
    std::vector<size_t> cylindersToRemove;

    m_ColManager.tris.resize(numFaces);
    for(TriangleCollider*& collider: m_ColManager.tris) {
        collider = new TriangleCollider();

        collider->ReadData(&reader);
        for(int j = 0; j < 3; j++) {
            collider->vertices[j].vertexBufferIndex = reader.ReadUInt16();
            uint16_t linkIndex = reader.ReadUInt16();
            collider->vertices[j].linkedFrame = m_ColManager.links[linkIndex].frame;
        }

        collider->plane.normal = reader.ReadVec3();
        collider->plane.distance = reader.ReadSingle();

        HierarchyEntry* entry = FindEntryByFrame(collider->vertices[0].linkedFrame);

        if(entry) {
            for (int j = 0; j < 3; j++) {
                S_vertex_3d* vertices = nullptr;
                if(collider->vertices[j].linkedFrame->GetType() == FRAME_VISUAL) {
                    I3D_visual* visual = (I3D_visual*)collider->vertices[j].linkedFrame;

                    if(visual->GetVisualType() == VISUAL_OBJECT || visual->GetVisualType() == VISUAL_LIT_OBJECT) {
                        I3D_object* obj = (I3D_object*)visual;

                        I3D_mesh_object* mesh = obj->GetMesh();

                        I3D_mesh_level* lod = mesh->GetLOD(0);

                        vertices = lod->LockVertices(1);
                        collider->vertices[j].vertexPos = vertices[collider->vertices[j].vertexBufferIndex].pos;
                        lod->UnlockVertices();
                    }
                }
            }

            if(!IsMeshColliderPresent(collider->vertices[0].linkedFrame)) {
                MeshCollider* meshCollider = new MeshCollider();
                meshCollider->type = collider->type;
                meshCollider->sortInfo = collider->sortInfo;
                meshCollider->flags = collider->flags;
                meshCollider->mtlId = collider->mtlId;
                meshCollider->linkedFrame = collider->vertices[0].linkedFrame;
                meshCollider->tris.push_back(collider);

                entry->colliders.push_back(meshCollider);
                m_ColManager.meshes.push_back(meshCollider);
                m_ColManager.colliders.push_back(meshCollider);
            } else {
                MeshCollider* meshCollider = GetMeshCollider(collider->vertices[0].linkedFrame);
                meshCollider->tris.push_back(collider);
            }
        }
        else {
            trisToRemove.push_back(i);
        }

        i++;
    }

    i = 0;

    m_ColManager.aabbs.resize(numAABBs);
    for(AABBCollider*& collider: m_ColManager.aabbs) {
        collider = new AABBCollider();
        
        collider->ReadData(&reader);
        uint32_t linkId = reader.ReadUInt32();
        collider->linkedFrame = m_ColManager.links[linkId].frame;
        collider->min = reader.ReadVec3();
        collider->max = reader.ReadVec3();

        HierarchyEntry* entry = FindEntryByFrame(collider->linkedFrame);
        if(entry) {
            entry->colliders.push_back(collider);
            
            m_ColManager.colliders.push_back(collider);
        }
        else {
            aabbsToRemove.push_back(i);
        }
        i++;
    }

    i = 0;

    m_ColManager.xtobbs.resize(numXTOBBs);
    for(XTOBBCollider*& collider: m_ColManager.xtobbs) {
        collider = new XTOBBCollider();

        collider->ReadData(&reader);
        uint32_t linkId = reader.ReadUInt32();
        collider->linkedFrame = m_ColManager.links[linkId].frame;
        collider->min = reader.ReadVec3();
        collider->max = reader.ReadVec3();
        collider->minExtent = reader.ReadVec3();
        collider->maxExtent = reader.ReadVec3();
        collider->transform = reader.ReadMatrix();
        collider->inverseTransform = reader.ReadMatrix();

        HierarchyEntry* entry = FindEntryByFrame(collider->linkedFrame);
        if(entry) {
            entry->colliders.push_back(collider);

            m_ColManager.colliders.push_back(collider);
        } else {
            xtobbsToRemove.push_back(i);
        }
        i++;
    }

    i = 0;

    m_ColManager.cylinders.resize(numCylinders);
    for(CylinderCollider*& collider: m_ColManager.cylinders) {
        collider = new CylinderCollider();

        collider->ReadData(&reader);
        uint32_t linkId = reader.ReadUInt32();
        collider->linkedFrame = m_ColManager.links[linkId].frame;
        collider->pos = reader.ReadVec2();
        collider->radius = reader.ReadSingle();

        HierarchyEntry* entry = FindEntryByFrame(collider->linkedFrame);
        if(entry) {
            entry->colliders.push_back(collider);

            m_ColManager.colliders.push_back(collider);
        } else {
            cylindersToRemove.push_back(i);
        }
        i++;
    }

    i = 0;

    m_ColManager.obbs.resize(numOBBs);
    for(OBBCollider*& collider: m_ColManager.obbs) {
        collider = new OBBCollider();

        collider->ReadData(&reader);
        uint32_t linkId = reader.ReadUInt32();
        collider->linkedFrame = m_ColManager.links[linkId].frame;
        collider->minExtent = reader.ReadVec3();
        collider->maxExtent = reader.ReadVec3();
        collider->transform = reader.ReadMatrix();
        collider->inverseTransform = reader.ReadMatrix();

        HierarchyEntry* entry = FindEntryByFrame(collider->linkedFrame);
        if(entry) {
            entry->colliders.push_back(collider);

            m_ColManager.colliders.push_back(collider);
        } else {
            obbsToRemove.push_back(i);
        }
        i++;
    }

    i = 0;

    m_ColManager.spheres.resize(numSpheres);
    for(SphereCollider*& collider: m_ColManager.spheres) {
        collider = new SphereCollider();

        collider->ReadData(&reader);
        uint32_t linkId = reader.ReadUInt32();
        collider->linkedFrame = m_ColManager.links[linkId].frame;
        collider->pos = reader.ReadVec3();
        collider->radius = reader.ReadSingle();

        HierarchyEntry* entry = FindEntryByFrame(collider->linkedFrame);
        if(entry) {
            entry->colliders.push_back(collider);

            m_ColManager.colliders.push_back(collider);
        } else {
            spheresToRemove.push_back(i);
        }
        i++;
    }

    // Remove unreferenced collisions
    if(!trisToRemove.empty()) {
        for(size_t i = trisToRemove.size() - 1; i > 0; i--) {
            delete m_ColManager.tris[i];

            m_ColManager.tris.erase(m_ColManager.tris.begin() + i);
        }
    }

    if(!aabbsToRemove.empty()) {
        for(size_t i = aabbsToRemove.size() - 1; i > 0; i--) {
            delete m_ColManager.aabbs[i];

            m_ColManager.aabbs.erase(m_ColManager.aabbs.begin() + i);
        }
    }

    if(!xtobbsToRemove.empty()) {
        for(size_t i = xtobbsToRemove.size() - 1; i > 0; i--) {
            delete m_ColManager.xtobbs[i];

            m_ColManager.xtobbs.erase(m_ColManager.xtobbs.begin() + i);
        }
    }

    if(!cylindersToRemove.empty()) {
        for(size_t i = cylindersToRemove.size() - 1; i > 0; i--) {
            delete m_ColManager.cylinders[i];

            m_ColManager.cylinders.erase(m_ColManager.cylinders.begin() + i);
        }
    }

    if(!obbsToRemove.empty()) {
        for(size_t i = obbsToRemove.size() - 1; i > 0; i--) {
            delete m_ColManager.obbs[i];

            m_ColManager.obbs.erase(m_ColManager.obbs.begin() + i);
        }
    }

    if(!spheresToRemove.empty()) {
        for(size_t i = spheresToRemove.size() - 1; i > 0; i--) {
            delete m_ColManager.spheres[i];

            m_ColManager.spheres.erase(m_ColManager.spheres.begin() + i);
        }
    }

    reader.Seek(sizeof(int32_t), SEEK_CUR); // Skip 1 int32 for VERSION_MAFIA

    uint32_t numCells = grid.length * grid.width; // No height for VERSION_MAFIA
    m_ColManager.cells.resize(numCells);
    for(GridCell& cell: m_ColManager.cells) {
        cell.numVolumes = reader.ReadUInt32();
        reader.ReadInt32();
        cell.height = reader.ReadSingle();
        cell.width = reader.ReadSingle(); // Additional unknown for VERSION_MAFIA
        if(cell.numVolumes) {
            cell.references.resize(cell.numVolumes);
            for(GridCell::Reference& ref: cell.references) {
                int32_t packed = reader.ReadInt32();
                ref.volumeBufferOffset = packed & 0x00FFFFFF;
                ref.volumeType = (packed >> 24) & 0xFF;
            }
            cell.flags.resize(cell.numVolumes);
            reader.Read(cell.flags.data(), cell.numVolumes);
            int padding = static_cast<int>(cell.numVolumes % 4);
            if(padding != 0) { reader.Seek(4 - padding, SEEK_CUR); }
        }
    }

    return true;
}

bool SceneEditor::ReadChunk(E_CHUNK_TYPE chunkType, ChunkReader& chunk) {
    switch(chunkType) {
    case CT_VERSION: {
        const auto versionMaj = chunk.Read<uint16_t>();
        const auto versionMin = chunk.Read<uint16_t>();
        auto copyright = chunk.ReadNullTerminatedString();
        uint8_t byte = chunk.Read<uint8_t>();
    } break;

        /*case CT_CAMERA_FOV: {
        const auto fov = chunk.Read<float>();
        m_CameraFOV = fov;
        //m_Camera->SetFOV(fov);
    } break;*/

    case CT_CAMERA_RANGE: {
        const auto range = chunk.Read<float>();

        m_Camera->SetRange(0.2f, range);
    } break;

    case CT_SCENE_CLIPPING_RANGE: {
        const auto nearRange = chunk.Read<float>();
        const auto farRange = chunk.Read<float>();

        m_Scene->SetBackdropRange(nearRange, farRange);
    } break;

    case CT_MODIFICATIONS: {
        while(chunk) {
            auto type = ++chunk;
            switch(type) {
            case CT_MODIFICATION: {
                I3D_frame* frame = nullptr;
                I3D_FRAME_TYPE frameType = FRAME_NULL;
                I3D_VISUAL_TYPE visualType = VISUAL_OBJECT;
                I3D_frame* parentFrame = nullptr;
                S_vector pos, worldPos, scale;
                bool posOK = false, worldPosOK, scaleOK = false;
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
                        worldPos = chunk.Read<S_vector>();
                        worldPosOK = true;
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

                                if(parent) { parentFrame = parent; }
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
                            auto fileName = chunk.ReadNullTerminatedString();
                            if(model->Open(("Models\\" + fileName).c_str()) == I3D_OK) { g_ModelsMap.insert({model, fileName}); }
                        }
                    } break;

                        // NOTE: Occluders are disabled, because they fucking crash for example in FREERIDE

                        /*case CT_MODIFY_OCCLUDER: {
                        uint32_t numVertices = chunk.Read<uint32_t>();
                        if(numVertices) {
                            S_vector* vertices = new S_vector[numVertices];
                            chunk.Read(vertices, sizeof(S_vector) * numVertices);

                            uint32_t numIndices = chunk.Read<uint32_t>() * 3;
                            if(numIndices) {
                                uint16_t* indices = new uint16_t[numIndices];
                                chunk.Read(indices, sizeof(uint16_t) * numIndices);

                                if(frame && frame->GetType() == FRAME_OCCLUDER) {
                                    I3D_occluder* occluder = (I3D_occluder*)frame;

                                    occluder->Build(vertices, numVertices, indices, numIndices, 0);
                                }

                                delete[] indices;
                            }

                            delete[] vertices;
                        }
                    } break;*/

                    case CT_MODIFY_VISUAL: {
                        I3D_visual* visual = (I3D_visual*)frame;
                        I3D_VISUAL_TYPE visualType = static_cast<I3D_VISUAL_TYPE>(chunk.Read<uint32_t>());
                        if(visual && frame->m_eFrameType == FRAME_VISUAL && visual->GetVisualType() != visualType) {
                            bool ok = false;
                            I3D_visual* newVisual = m_3DDriver->CreateVisual(visualType);
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

                                    //frame->Release();

                                    frame = newVisual;
                                    ok = true;
                                }
                            }
                        }
                    } break;

                    case CT_MODIFY_LIT_OBJECT: {
                        I3D_visual* visual = (I3D_visual*)frame;

                        if(visual && frame->GetType() == FRAME_VISUAL && visual->GetVisualType() == VISUAL_LIT_OBJECT) {
                            I3D_lit_object* object = (I3D_lit_object*)visual;

                            S_vector pos{0, 0, 0};

                            LS3D_RESULT res = object->Load(chunk.GetReader()->GetDescriptor());

                            if(I3D_FAIL(res)) { debugPrintf("Failed to load lightmap data for %s: %s", frame->GetName(), I3DGetResultMessage(res)); }
                        }
                    } break;

                    case CT_MODIFY_LIGHT: {
                        I3D_light* light = (I3D_light*)frame;
                        while(chunk) {
                            ChunkType lightChunkType = ++chunk;
                            switch(lightChunkType) {
                            case CT_LIGHT_TYPE: {
                                if(light) light->SetLightType(static_cast<I3D_LIGHTTYPE>(chunk.Read<uint32_t>()));
                            } break;

                            case CT_LIGHT_POWER: {
                                if(light) light->SetPower(chunk.Read<float>());
                            } break;

                            case CT_LIGHT_CONE: {
                                if(light) light->SetCone(chunk.Read<float>(), chunk.Read<float>());
                            } break;

                            case CT_LIGHT_RANGE: {
                                if(light) light->SetRange(chunk.Read<float>(), chunk.Read<float>());
                            } break;

                            case CT_LIGHT_MODE: {
                                if(light) light->SetMode(chunk.Read<uint32_t>());
                            } break;

                            case CT_LIGHT_SECTOR: {
                                auto sectorName = chunk.ReadNullTerminatedString();

                                I3D_sector* sector = nullptr;

                                if(sectorName == "Backdrop sector") {
                                    sector = m_BackdropSector;
                                } else if(sectorName == "Primary sector") {
                                    sector = m_PrimarySector;
                                } else {
                                    sector = (I3D_sector*)m_Scene->FindFrame(sectorName.c_str(), ENUMF_SECTOR);
                                }

                                if(sector && light) { sector->AddLight(light); }
                            } break;

                            case CT_COLOR: {
                                if(light) light->SetColor2(chunk.Read<S_vector>());
                            } break;

                            default: break;
                            }
                            --chunk;
                        }
                    } break;

                    case CT_MODIFY_SOUND: {
                        I3D_sound* sound = (I3D_sound*)frame;
                        while(chunk) {
                            ChunkType soundChunkType = ++chunk;
                            switch(soundChunkType) {
                            case CT_NAME: {
                                if(sound) {
                                    auto fileName = chunk.ReadNullTerminatedString();

                                    if(sound->Open(("Sounds\\" + fileName).c_str(), 0, nullptr, nullptr) == I3D_OK) { g_SoundsMap.insert({sound, fileName}); }
                                }
                            } break;

                            case CT_SOUND_TYPE: {
                                if(sound) sound->SetSoundType(static_cast<I3D_SOUNDTYPE>(chunk.Read<uint32_t>()));
                            } break;

                            case CT_SOUND_CONE: {
                                if(sound) sound->SetCone(chunk.Read<float>(), chunk.Read<float>());
                            } break;

                            case CT_SOUND_VOLUME: {
                                if(sound) sound->SetVolume(chunk.Read<float>());
                            } break;

                            case CT_SOUND_OUTVOL: {
                                if(sound) sound->SetOutVol(chunk.Read<float>());
                            } break;

                            case CT_SOUND_RANGE: {
                                if(sound) {
                                    float innerRadius = chunk.Read<float>();
                                    float outerRadius = chunk.Read<float>();
                                    float innerFalloff = chunk.Read<float>();
                                    float outerFalloff = chunk.Read<float>();

                                    sound->SetRange(innerRadius, outerRadius, innerFalloff, outerFalloff);
                                }
                            } break;

                            case CT_SOUND_LOOP: {
                                if(sound) sound->SetLoop(true);
                            } break;

                            case CT_SOUND_SECTOR: {
                                auto sectorName = chunk.ReadNullTerminatedString();

                                I3D_sector* sector = nullptr;

                                if(sectorName == "Backdrop sector") {
                                    sector = m_BackdropSector;
                                } else if(sectorName == "Primary sector") {
                                    sector = m_PrimarySector;
                                } else {
                                    sector = (I3D_sector*)m_Scene->FindFrame(sectorName.c_str(), ENUMF_SECTOR);
                                }

                                if(sector && sound) { sector->AddSound(sound); }
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
                    if(parentFrame) { frame->LinkTo(parentFrame); }

                    if(posOK) frame->SetPos(pos);

                    if(rotOK) frame->SetRot(rot);

                    if(scaleOK) frame->SetScale(scale);

                    if(isHidden) { frame->SetOn(false); }

                    if(created && !parentFrame) {
                        S_vector pos = frame->GetWorldPos();

                        m_Scene->SetFrameSectorPos(frame, pos);
                    }

                    if(frame->GetType() != FRAME_OCCLUDER) frame->Update();

                    if(created) {
                        m_Scene->AddFrame(frame);
                        m_CreatedFrames.push_back(frame);
                    }

                    m_Frames.push_back(frame);

                    /*if (parentFrame)
						frame->LinkTo(parentFrame, 1);
					else if (frame != m_PrimarySector && frame != m_BackdropSector) {
						I3D_sector* sector = m_Scene->GetSector2(*(S_vector*)&frame->m_mWorldMat.e[12], nullptr);
						if (sector) {
							frame->LinkTo(sector, 1);
						}
					}*/
                }
            } break;

            default: break;
            }
            --chunk;
        }
    } break;

    case CT_ACTORS: {
        while(chunk) {
            auto type = ++chunk;
            if(type == CT_ACTOR) {
                std::string actorName;
                Actor* actor = nullptr;
                ActorType actorType;

                while(chunk) {
                    type = ++chunk;

                    switch(type) {
                    case CT_ACTOR_NAME: {
                        actorName = chunk.ReadNullTerminatedString();
                    } break;
                    case CT_ACTOR_TYPE: {
                        actorType = chunk.Read<ActorType>();

                        I3D_frame* frame = m_Scene->FindFrame(actorName.c_str(), ENUMF_ALL);

                        if(frame) { actor = CreateActor(actorType, frame); }
                    } break;
                    case CT_ACTOR_DATA: {
                        if(actor) {
                            //actor->SetName(actorName);
                            actor->OnLoad(&chunk);
                        }
                    } break;
                    }

                    --chunk;
                }
            }
            --chunk;
        }
    } break;

    case CT_SCRIPTS: {
        while(chunk) {
            auto type = ++chunk;

            if(type == CT_SCRIPT) {
                GameScript& script = m_Scripts.emplace_back();
                uint8_t unk = chunk.Read<uint8_t>();
                script.name = chunk.ReadString();
                script.script = chunk.ReadString();
            }
        }
    } break;

    default: break;
    }

    return true;
}

uint8_t SetBitsFromCount(uint8_t count) {
    if(count > 8) return 0;
    return (1U << count) - 1;
}

void SceneEditor::WriteFrame(I3D_frame* frame, ChunkWriter& chunk, bool forceWrite) {
    bool created = std::find(m_CreatedFrames.begin(), m_CreatedFrames.end(), frame) != m_CreatedFrames.end();

    if(std::find(m_Frames.begin(), m_Frames.end(), frame) != m_Frames.end() || forceWrite) {
        chunk += CT_MODIFICATION;
        {
            if(created) { chunk.WriteUIntChunk(CT_FRAME_TYPE, frame->GetType()); }

            I3D_visual* visual = (I3D_visual*)frame;

            if(frame->GetType() == FRAME_VISUAL && created) {
                chunk.WriteUIntChunk(CT_FRAME_SUBTYPE, visual->GetVisualType() == VISUAL_LIT_OBJECT ? VISUAL_OBJECT : visual->GetVisualType());
            }

            chunk.WriteStringChunk(CT_NAME, frame->GetName());
            chunk.WriteVec3Chunk(CT_POSITION, frame->GetPos());
            chunk.WriteVec3Chunk(CT_WORLD_POSITION, frame->GetWorldPos());
            chunk.WriteQuatChunk(CT_QUATERNION, frame->GetRot());
            chunk.WriteVec3Chunk(CT_SCALE, frame->GetScale());

            I3D_frame* parent = frame->GetParent();

            if(parent) {
                chunk += CT_LINK;
                {
                    chunk.WriteStringChunk(CT_NAME, parent->GetName());
                    --chunk;
                }
            }

            if(!frame->IsOn()) {
                chunk += CT_HIDDEN;
                --chunk;
            }

            if(frame->GetType() == FRAME_MODEL && created) { chunk.WriteStringChunk(CT_MODIFY_MODELFILENAME, g_ModelsMap[(I3D_model*)frame]); }

            if(frame->GetType() == FRAME_VISUAL) {
                chunk.WriteUIntChunk(CT_MODIFY_VISUAL, visual->GetVisualType());

                if(visual->GetVisualType() == VISUAL_LIT_OBJECT) {
                    I3D_lit_object* obj = (I3D_lit_object*)visual;
                    chunk += CT_MODIFY_LIT_OBJECT;
                    {
                        if(I3D_FAIL(obj->CustomSave(chunk.GetWriter()))) {
                            debugPrintf("Frame %s failed to write LM data - the scene file will be corrupted.", obj->GetName());
                            m_SavedProperly = false;
                        }
                        --chunk;
                    }
                }
            }

            if(frame->GetType() == FRAME_LIGHT) {
                I3D_light* light = (I3D_light*)frame;

                chunk += CT_MODIFY_LIGHT;
                {
                    float nearRange = 0, farRange = 0;

                    chunk.WriteUIntChunk(CT_LIGHT_TYPE, light->GetLightType());
                    chunk.WriteFloatChunk(CT_LIGHT_POWER, light->GetPower());
                    light->GetCone(nearRange, farRange);
                    chunk.WriteVec2Chunk(CT_LIGHT_CONE, {nearRange, farRange});
                    light->GetRange(nearRange, farRange);
                    chunk.WriteVec2Chunk(CT_LIGHT_RANGE, {nearRange, farRange});
                    chunk.WriteVec3Chunk(CT_COLOR, light->GetColor());
                    chunk.WriteUIntChunk(CT_LIGHT_MODE, light->GetMode());

                    for(int i = light->NumLightSectors(); i--;) {
                        I3D_sector* sector = light->GetLightSectors()[i];
                        chunk.WriteStringChunk(CT_LIGHT_SECTOR, sector->GetName());
                    }

                    --chunk;
                }
            }

            if(frame->GetType() == FRAME_SOUND) {
                I3D_sound* sound = (I3D_sound*)frame;

                float nearRange = 0, farRange = 0, nearFalloff = 0, farFalloff = 0;

                chunk += CT_MODIFY_SOUND;
                {
                    chunk.WriteStringChunk(CT_NAME, g_SoundsMap[sound]);
                    chunk.WriteUIntChunk(CT_SOUND_TYPE, sound->GetSoundType());
                    chunk.WriteFloatChunk(CT_SOUND_VOLUME, sound->GetVolume());
                    chunk += CT_SOUND_RANGE;
                    {
                        sound->GetRange(nearRange, farRange, nearFalloff, farFalloff);
                        chunk.Write(nearRange);
                        chunk.Write(farRange);
                        chunk.Write(nearFalloff);
                        chunk.Write(farFalloff);
                        --chunk;
                    }
                    if(sound->IsLoop()) {
                        chunk += CT_SOUND_LOOP;
                        --chunk;
                    }
                    size_t numSectors = sound->NumSoundSectors();

                    for(uint32_t i = 0; i < numSectors; i++) {
                        I3D_sector* sector = sound->GetSoundSectors()[i];
                        chunk.WriteStringChunk(CT_SOUND_SECTOR, sector->GetName());
                    }

                    --chunk;
                }
            }

            --chunk;
        }
    }
}

void SceneEditor::WriteFrameRecursively(I3D_frame* frame, ChunkWriter& chunk, bool writeParent) {
    if(!frame) return;

    if(writeParent) { WriteFrame(frame, chunk); }

    if(frame->NumChildren() == 0) return;

    for(uint32_t i = 0; i < frame->NumChildren(); i++) {
        auto child = frame->GetChild(i);

        // NOTE: For models, we don't really want to write contents of the model, unless there's an actual linked frame.
        if((child->GetType() == FRAME_VISUAL || child->GetType() == FRAME_DUMMY || child->GetType() == FRAME_TARGET || child->GetType() == FRAME_JOINT) &&
           frame->GetType() == FRAME_MODEL) {
            WriteFrame(child, chunk);
        } else {
            WriteFrameRecursively(child, chunk, true);
        }
    }
}

void SceneEditor::WriteSceneBin(const std::string& fileName) {
    BinaryWriter writer(fileName);
    if(writer.IsOpen()) {
        ChunkWriter chunk(&writer);

        chunk += CT_BASECHUNK;
        {
            chunk += CT_VERSION;
            {
                chunk.Write<uint16_t>(MISSION_VER_MAJOR);
                chunk.Write<uint16_t>(MISSION_VER_MINOR);
                chunk.WriteNullTerminatedString(m_MissionFileSignature);
                chunk.Write<uint8_t>(0);
                --chunk;
            }

            chunk.WriteVec3Chunk(CT_SCENE_BGND_COLOR, m_Scene->GetClearColor());
            chunk.WriteFloatChunk(CT_CAMERA_FOV, GAME_FOV);
            float nearRange = 0, farRange = 0;
            m_Camera->GetRange(nearRange, farRange);
            chunk.WriteFloatChunk(CT_CAMERA_RANGE, farRange);
            m_Scene->GetBackdropRange(nearRange, farRange);
            chunk.WriteVec2Chunk(CT_SCENE_CLIPPING_RANGE, {nearRange, farRange});

            chunk += CT_MODIFICATIONS;
            {
                WriteFrameRecursively(m_PrimarySector, chunk, true);
                WriteFrameRecursively(m_BackdropSector, chunk);

                --chunk;
            }

            chunk += CT_ACTORS;
            {
                for(auto actor: m_Actors) {
                    actor->Save(&chunk);
                }

                --chunk;
            }

            chunk += CT_SCRIPTS;
            {
                for(auto script: m_Scripts) {
                    chunk += CT_SCRIPT;
                    {
                        chunk.Write<uint8_t>(0);
                        chunk.WriteString(script.name);
                        chunk.WriteString(script.script);

                        --chunk;
                    }
                }

                --chunk;
            }

            --chunk;
        }
    }
}

void SceneEditor::WriteCacheBin(const std::string& fileName) {
    if(m_CacheParts.empty()) return;

    BinaryWriter writer(fileName);
    if(writer.IsOpen()) {
        ChunkWriter chunk(&writer);

        chunk.Ascend(CT_CITY);
        {
            for(auto& part: m_CacheParts) {
                chunk.Ascend(CT_PART);
                {
                    chunk.Write(part.spherePos);
                    chunk.Write(part.sphereRadius);
                    chunk.Write(part.leftBottomCorner);
                    chunk.Write(part.leftSide);
                    chunk.Write(part.rightBottomCorner);
                    chunk.Write(part.rightSide);
                    chunk.Write(part.leftTopCorner);
                    chunk.Write(part.bottomSide);
                    chunk.Write(part.rightTopCorner);
                    chunk.Write(part.topSide);

                    for(auto model: part.models) {
                        chunk.Ascend(CT_FRAME);
                        {
                            chunk.WriteString(g_ModelsMap[model]);
                            chunk.Write(model->GetPos());
                            chunk.Write(model->GetRot());
                            chunk.Write(model->GetScale());
                            chunk.Write(0);
                            chunk.Write(model->GetScale());

                            --chunk;
                        }
                    }

                    --chunk;
                }
            }

            --chunk;
        }
    }
}

void SceneEditor::WriteRoadBin(const std::string& fileName) {
    if(m_RoadCrosspoints.empty() || m_RoadWaypoints.empty()) return;

    BinaryWriter writer(fileName);
    if(writer.IsOpen()) {
        writer.WriteUInt32(2);

        writer.WriteUInt32(m_RoadCrosspoints.size());
        for(auto& pair: m_RoadCrosspoints) {
            RoadCrosspoint& cross = pair.second;
            writer.WriteVec3(cross.pos);
            writer.WriteBoolean(cross.hasSemaphore);
            writer.Write(cross.pad0, 3);
            writer.WriteSingle(SPEED_KMH_TO_GAME(cross.speed));

            for(int j = 0; j < 4; j++) {
                writer.WriteUInt16(cross.waypointLinks[j]);
            }

            for(int j = 0; j < 4; j++) {
                RoadDirectionLink& link = cross.directionLinks[j];

                writer.WriteUInt16(link.crosspointLink);
                writer.WriteUInt16(link.unk1);
                writer.WriteSingle(link.distance);
                writer.WriteSingle(RAD(link.angle));
                writer.WriteUInt16(link.unk2);
                writer.WriteUInt32(link.priority);
                writer.WriteUInt16(link.unk3);

                for(int k = 0; k < 4; k++) {
                    RoadLane& lane = link.lanes[k];

                    writer.WriteUInt16(lane.unk1);
                    writer.WriteUInt8(lane.type);
                    writer.WriteUInt8(lane.unk2);
                    writer.WriteSingle(lane.distance);
                }
            }
        }

        writer.WriteUInt32(m_RoadWaypoints.size());
        for(auto& pair: m_RoadWaypoints) {
            RoadWaypoint& point = pair.second;
            writer.WriteVec3(point.pos);
            writer.WriteSingle(SPEED_KMH_TO_GAME(point.speed));
            writer.WriteUInt16(point.prevWaypoint);
            writer.WriteUInt16(point.nextWaypoint);
            writer.WriteUInt16(point.prevCrosspoint);
            writer.WriteUInt16(point.nextCrosspoint);
        }
    }
}

void SceneEditor::WriteCheckBin(const std::string& fileName) {
    if(m_WebNodes.empty()) return;

    BinaryWriter writer(fileName);

    if(writer.IsOpen()) {
        writer.WriteUInt32(0x1ABCEDF);
        writer.WriteUInt32(m_WebNodes.size());
        for(auto& pair: m_WebNodes) {
            WebNode& node = pair.second;
            writer.WriteVec3(node.pos);
            writer.WriteUInt16(node.type);
            writer.WriteUInt16(node.id);
            writer.WriteUInt16(node.radius);
            writer.Write(node.unk, 10);
            writer.WriteUInt8(node.numEnterLinks);
            writer.WriteUInt8(node.numExitLinks);
        }

        for(auto& pair: m_WebNodes) {
            WebNode& node = pair.second;

            for(WebConnection& conn: node.links) {
                writer.WriteUInt16(conn.endPoint);
                writer.WriteUInt16(conn.type);
                writer.WriteSingle(conn.length);
            }
        }
    }
}

void SceneEditor::WriteTreeKlz(const std::string& fileName) {
    if(m_ColManager.colliders.empty()) return;

    BinaryWriter writer(fileName);
    if(writer.IsOpen()) {
        // Write the file header
        writer.WriteFixedString("GifC", 4);
        writer.WriteUInt32(5); // 5 = VERSION_MAFIA - I think that makes sense, since this is an editor for MAFIA. For HD2, take a look at Lutsip instead. :)

        size_t headerPlaceholder = writer.GetCurPos();
        writer.WriteUInt32(0); // Bogus gridDataOffset
        writer.WriteUInt32(0); // Bpgus numLinks
        writer.WriteUInt32(379); // TODO: Find out what that value is used for
        writer.WriteUInt32(0);

        m_ColManager.links.clear();
        // Collect the frame links
        std::unordered_map<I3D_frame*, uint32_t> frameToId;
        for(Collider* collider: m_ColManager.colliders) {
            switch(collider->type) {
            case Collider::VOLUME_FACE0:
            case Collider::VOLUME_FACE1:
            case Collider::VOLUME_FACE2:
            case Collider::VOLUME_FACE3:
            case Collider::VOLUME_FACE4:
            case Collider::VOLUME_FACE5:
            case Collider::VOLUME_FACE6:
            case Collider::VOLUME_FACE7: {
                MeshCollider* mesh = (MeshCollider*)collider;

                for(TriangleCollider* tri: mesh->tris) {
                    for(int i = 0; i < 3; i++) {
                        I3D_frame* frame = tri->vertices[i].linkedFrame;

                        if(frameToId.find(frame) == frameToId.end()) {
                            uint32_t newId = static_cast<uint32_t>(m_ColManager.links.size());
                            CollisionLink::LinkType linkType = CollisionLink::LINK_SURFACE;
                            m_ColManager.links.push_back({linkType, frame});
                            frameToId[frame] = newId; // Auto-index
                        }
                    }
                }
            } break;
            case Collider::VOLUME_AABB: {
                AABBCollider* aabb = (AABBCollider*)collider;

                I3D_frame* frame = aabb->linkedFrame;

                if(frameToId.find(frame) == frameToId.end()) {
                    uint32_t newId = static_cast<uint32_t>(m_ColManager.links.size());
                    CollisionLink::LinkType linkType = CollisionLink::LINK_VOLUME;
                    m_ColManager.links.push_back({linkType, frame});
                    frameToId[frame] = newId; // Auto-index
                }
            } break;
            case Collider::VOLUME_XTOBB: {
                XTOBBCollider* xtobb = (XTOBBCollider*)collider;

                I3D_frame* frame = xtobb->linkedFrame;

                if(frameToId.find(frame) == frameToId.end()) {
                    uint32_t newId = static_cast<uint32_t>(m_ColManager.links.size());
                    CollisionLink::LinkType linkType = CollisionLink::LINK_VOLUME;
                    m_ColManager.links.push_back({linkType, frame});
                    frameToId[frame] = newId; // Auto-index
                }
            } break;
            case Collider::VOLUME_CYLINDER: {
                CylinderCollider* cylinder = (CylinderCollider*)collider;

                I3D_frame* frame = cylinder->linkedFrame;

                if(frameToId.find(frame) == frameToId.end()) {
                    uint32_t newId = static_cast<uint32_t>(m_ColManager.links.size());
                    CollisionLink::LinkType linkType = CollisionLink::LINK_VOLUME;
                    m_ColManager.links.push_back({linkType, frame});
                    frameToId[frame] = newId; // Auto-index
                }
            } break;
            case Collider::VOLUME_OBB: {
                OBBCollider* obb = (OBBCollider*)collider;

                I3D_frame* frame = obb->linkedFrame;

                if(frameToId.find(frame) == frameToId.end()) {
                    uint32_t newId = static_cast<uint32_t>(m_ColManager.links.size());
                    CollisionLink::LinkType linkType = CollisionLink::LINK_VOLUME;
                    m_ColManager.links.push_back({linkType, frame});
                    frameToId[frame] = newId; // Auto-index
                }
            } break;
            case Collider::VOLUME_SPHERE: {
                SphereCollider* sphere = (SphereCollider*)collider;

                I3D_frame* frame = sphere->linkedFrame;

                if(frameToId.find(frame) == frameToId.end()) {
                    uint32_t newId = static_cast<uint32_t>(m_ColManager.links.size());
                    CollisionLink::LinkType linkType = CollisionLink::LINK_VOLUME;
                    m_ColManager.links.push_back({linkType, frame});
                    frameToId[frame] = newId; // Auto-index
                }
            } break;
            }
        }

        // Write bogus offsets to frame links
        uint32_t offsetsPlaceholder = static_cast<uint32_t>(writer.GetCurPos());
        for(size_t i = 0; i < m_ColManager.links.size(); ++i) {
            writer.WriteUInt32(0);
        }

        // Write frame links
        std::vector<uint32_t> actualLinkOffsets(m_ColManager.links.size());
        for(size_t i = 0; i < m_ColManager.links.size(); ++i) {
            actualLinkOffsets[i] = static_cast<uint32_t>(writer.GetCurPos());
            writer.WriteUInt32(m_ColManager.links[i].type);
            std::string name = m_ColManager.links[i].frame->GetName();
            writer.WriteNullTerminatedString(name);
            size_t len = name.length() + 1;
            int padding = static_cast<int>((len % 4) ? 4 - (len % 4) : 0);
            for(int p = 0; p < padding; ++p) {
                writer.WriteUInt8(0);
            }
        }

        // Write actual offsets to frame links
        writer.Seek(offsetsPlaceholder, SEEK_SET);
        for(size_t i = 0; i < m_ColManager.links.size(); ++i) {
            writer.WriteUInt32(actualLinkOffsets[i]);
        }
        writer.Seek(0, SEEK_END);

        // Write actual gridDataOffset and numLinks to the header
        uint32_t gridDataOffset = static_cast<uint32_t>(writer.GetCurPos());
        writer.Seek(headerPlaceholder, SEEK_SET);
        writer.WriteUInt32(gridDataOffset);
        writer.WriteUInt32(m_ColManager.links.size());
        writer.Seek(0, SEEK_END);

        m_ColManager.GenerateGrid();

        // Write the grid
        writer.WriteVec2(m_ColManager.grid.min);
        writer.WriteVec2(m_ColManager.grid.max);
        writer.WriteVec2(m_ColManager.grid.cellSize);
        writer.WriteUInt32(m_ColManager.grid.width);
        writer.WriteUInt32(m_ColManager.grid.length);
        writer.WriteSingle(20.0f);
        writer.WriteVec3({0, 0, 0});

        writer.WriteUInt32(m_ColManager.tris.size());
        writer.WriteUInt32(0); // Probably an reserved pointer
        writer.WriteUInt32(m_ColManager.xtobbs.size());
        writer.WriteUInt32(0); // Probably an reserved pointer
        writer.WriteUInt32(m_ColManager.aabbs.size());
        writer.WriteUInt32(0); // Probably an reserved pointer
        writer.WriteUInt32(m_ColManager.spheres.size());
        writer.WriteUInt32(0); // Probably an reserved pointer
        writer.WriteUInt32(m_ColManager.obbs.size());
        writer.WriteUInt32(0); // Probably an reserved pointer
        writer.WriteUInt32(m_ColManager.cylinders.size());
        writer.WriteUInt32(0); // Probably an reserved pointer

        for(int i = 0; i < 2; ++i) {
            writer.WriteInt32(0); // 2 integers to fill up the space
        }

        for(float val: m_ColManager.grid.bounds.x) {
            writer.WriteSingle(val);
        }
        for(float val: m_ColManager.grid.bounds.y) {
            writer.WriteSingle(val);
        }

        writer.WriteInt32(0); // Bogus integer to fill up the space

        std::vector<ColliderInfo> colliderInfos;

        // NOTE: I need to undefine "min" here in order to make std::min work
        #undef min

        // Write face collisions
        for(const TriangleCollider* collider: m_ColManager.tris) {
            colliderInfos.push_back({writer.GetCurPos(), collider->type});
            collider->WriteData(&writer);
            for(int j = 0; j < 3; ++j) {
                writer.WriteUInt16(collider->vertices[j].vertexBufferIndex);
                uint32_t linkId = frameToId[collider->vertices[0].linkedFrame];
                writer.WriteUInt16(linkId);
            }
            writer.WriteVec3(collider->plane.normal);
            writer.WriteSingle(collider->plane.distance);
        }

        // Write AABB collisions
        for(const AABBCollider* collider: m_ColManager.aabbs) {
            colliderInfos.push_back({writer.GetCurPos(), collider->type});
            collider->WriteData(&writer);
            uint32_t linkId = frameToId[collider->linkedFrame];
            writer.WriteUInt32(linkId);
            writer.WriteVec3(collider->min);
            writer.WriteVec3(collider->max);
        }

        // Write XTOBB collisions
        for(const XTOBBCollider* collider: m_ColManager.xtobbs) {
            colliderInfos.push_back({writer.GetCurPos(), collider->type});
            collider->WriteData(&writer);
            uint32_t linkId = frameToId[collider->linkedFrame];
            writer.WriteUInt32(linkId);
            writer.WriteVec3(collider->min);
            writer.WriteVec3(collider->max);
            writer.WriteVec3(collider->minExtent); // VECTOR3 for VERSION_MAFIA
            writer.WriteVec3(collider->maxExtent);
            writer.WriteMatrix(collider->transform);
            writer.WriteMatrix(collider->inverseTransform);
        }

        // Write cylinder collisions
        for(const CylinderCollider* collider: m_ColManager.cylinders) {
            colliderInfos.push_back({writer.GetCurPos(), collider->type});
            collider->WriteData(&writer);
            uint32_t linkId = frameToId[collider->linkedFrame];
            writer.WriteUInt32(linkId);
            writer.WriteVec2(collider->pos);
            writer.WriteSingle(collider->radius);
        }

        // Write OBB collisions
        for(const OBBCollider* collider: m_ColManager.obbs) {
            colliderInfos.push_back({writer.GetCurPos(), collider->type});
            collider->WriteData(&writer);
            uint32_t linkId = frameToId[collider->linkedFrame];
            writer.WriteUInt32(linkId);
            writer.WriteVec3(collider->minExtent);
            writer.WriteVec3(collider->maxExtent);
            writer.WriteMatrix(collider->transform);
            writer.WriteMatrix(collider->inverseTransform);
        }

        // Write sphere collisions
        for(const SphereCollider* collider: m_ColManager.spheres) {
            colliderInfos.push_back({writer.GetCurPos(), collider->type});
            collider->WriteData(&writer);
            uint32_t linkId = frameToId[collider->linkedFrame];
            writer.WriteUInt32(linkId);
            writer.WriteVec3(collider->pos);
            writer.WriteSingle(collider->radius);
        }

        writer.WriteInt32(0); // Bogus integer to fill up the space

        // NOTE: That one is very, VERY FUCKING likely to be absolutely FUCKED
        // TODO: Make sure the game doesn't crash every fucking time
        m_ColManager.GenerateCells(colliderInfos);

        // Write grid cells
        for(const GridCell& cell: m_ColManager.cells) {
            writer.WriteUInt32(cell.numVolumes);
            writer.WriteSingle(cell.height);
            writer.WriteSingle(cell.width);
            writer.WriteInt32(-971227136);
            if(cell.numVolumes) {
                for(const GridCell::Reference& ref: cell.references) {
                    int32_t packed = (ref.volumeType << 24) | (ref.volumeBufferOffset & 0x00FFFFFF);
                    writer.WriteInt32(packed);
                }
                for(uint8_t flag: cell.flags) {
                    writer.WriteUInt8(flag);
                }
                int padding = static_cast<int>(cell.numVolumes % 4);
                if(padding != 0) {
                    padding = 4 - padding;
                    for(int p = 0; p < padding; ++p) {
                        writer.WriteUInt8(0);
                    }
                }
            }
        }
    }
}

void SceneEditor::Save(const std::string& path) {
    SelectFrame(m_SelectedFrame);

    m_ScriptEditor.SaveAll();

    m_SavedProperly = true;

    if(!std::filesystem::exists(path)) { std::filesystem::create_directories(path); }
    WriteSceneBin(path + "\\scene2.bin");
    WriteTreeKlz(path + "\\tree.klz");
    WriteRoadBin(path + "\\road.bin");
    WriteCheckBin(path + "\\check.bin");
    WriteCacheBin(path + "\\cache.bin");

    if(m_SavedProperly) {
        ShowPopupMessage("Scene has been saved.");
    } else {
        ShowPopupMessage("Scene has failed to save - check the log");
    }
}

void SceneEditor::ToggleInput(bool state) {
    //ImGui_ImplDInput_Data* inputData = ImGui_ImplDInput_GetData();

    m_MouseEnabled = state;

    m_KeyboardEnabled = state;

    if(m_MouseEnabled) {
        m_LS3DMouseDevice->SetCooperativeLevel(m_IGraph->GetMainHWND(), DISCL_EXCLUSIVE);
    } else {
        m_LS3DMouseDevice->SetCooperativeLevel(m_IGraph->GetMainHWND(), DISCL_NONEXCLUSIVE);
    }

    ShowCursor(!m_MouseEnabled);
}

void SceneEditor::Clear() {
    if(m_SceneLoaded) {
        ClearSelection();

        for(auto actor: m_Actors) {
            DeleteActor(actor->GetFrame());
        }

        EnableMenuItem(m_FileMenu, MCMD_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

        for(auto frame: m_CreatedFrames) {
            m_Scene->DeleteFrame(frame);
            frame->LinkTo(NULL);
            frame->Release();
        }

        for(auto part: m_CacheParts) {
            for(auto frame: part.models) {
                m_Scene->DeleteFrame(frame);
                frame->LinkTo(NULL);
                frame->Release();
            }
        }

        m_Frames.clear();
        m_CacheParts.clear();
        m_Actors.clear();
        m_Scripts.clear();
        m_WebNodes.clear();
        m_RoadWaypoints.clear();
        m_RoadCrosspoints.clear();
        m_AnimatedModels.clear();
        m_LightmapObjects.clear();

        m_ColManager.Clear();

        g_ModelsMap.clear();
        g_SoundsMap.clear();

        m_Scene->Close();

        m_SceneLoaded = false;

        // NOTE: Release the scene - I don't know how to delete frames loaded from scene.4ds (it either doesn't work or crashes), and this way worked.
        m_Scene->Release();
        m_Scene = nullptr;

        m_MissionPath = "";
        m_IGraph->SetAppName("Scene Editor");
    }
}