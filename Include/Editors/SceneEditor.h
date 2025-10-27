#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#include <chrono>
#include <IGraph/IGraph.h>
#include <ISND/ISND_driver.h>
#include <I3D/I3D_frame.h>
#include <I3D/I3D_driver.h>
#include <I3D/I3D_occluder.h>
#include <I3D/I3D_sector.h>
#include <I3D/I3D_visual.h>
#include <I3D/I3D_scene.h>
#include <I3D/I3D_camera.h>
#include <I3D/I3D_sound.h>
#include <I3D/I3D_light.h>
#include <I3D/I3D_model.h>
#include <I3D/I3D_dummy.h>
#include <I3D/I3D_material.h>
#include <I3D/I3D_animation_set.h>
#include <Game/Actor.h>
#include <IO/ChunkReader.hpp>
#include <IO/ChunkWriter.hpp>
#include <imgui.h>
#include <FA6.h>
#include <d3d8.h>
#include <ImGuizmo.h>
#include <Editors/ScriptEditor.h>
#include <set>

#include <Utils/MathUtils.h>

class I3D_lit_object;

#define MISSION_VER_MAJOR 5
#define MISSION_VER_MINOR 10

enum MenuCommand {
    // File menu
    MCMD_FILE_OPEN = 100,
    MCMD_FILE_SAVE,
    MCMD_FILE_CLOSE,

    // View menu
    MCMD_VIEW_WEBNODES = 200,
    MCMD_VIEW_ROADPOINTS,
    MCMD_VIEW_COLLISIONS,

    // Edit menu
    MCMD_EDIT_UNDO = 300,
    MCMD_EDIT_REDO,
    MCMD_EDIT_COPY,
    MCMD_EDIT_PASTE,
    MCMD_EDIT_DUPLICATE,
    MCMD_EDIT_DELETE,

    // Create -> Frame menu
    MCMD_CREATE_DUMMY = 400,
    MCMD_CREATE_MODEL,
    MCMD_CREATE_LIGHT,
    MCMD_CREATE_SOUND,

    // Create -> Actor menu
    MCMD_CREATE_PLAYER = 450,
    MCMD_CREATE_HUMAN,
    MCMD_CREATE_CAR,
    MCMD_CREATE_DETECTOR,
    MCMD_CREATE_RAILWAY,
    MCMD_CREATE_PEDMAN,
    MCMD_CREATE_TRAFFICMAN,
    MCMD_CREATE_PHYSICAL,

    // Create -> Road menu
    MCMD_CREATE_WAYPOINT = 460,
    MCMD_CREATE_CROSSPOINT,

    // Create menu
    MCMD_CREATE_WEBNODE = 480,
    MCMD_CREATE_SCRIPT,
    MCMD_CREATE_LIGHTMAP,

    // Help menu
    MCMD_HELP_ABOUT = 500
};

class SceneEditor {
  public:
#pragma pack(push, 1)
    struct RoadLane {
        uint16_t unk1;
        uint8_t type = 0;
        uint8_t unk2;
        float distance = 4.0f;
    };

    struct RoadDirectionLink {
        uint16_t crosspointLink = 0xFFFF;
        uint16_t unk1;
        float distance = 0.0f;
        float angle = 0.0f;
        uint16_t unk2;
        uint32_t priority = 100;
        uint16_t unk3;
        RoadLane lanes[4];
    };

    struct RoadCrosspoint {
        S_vector pos{0, 0, 0};
        bool hasSemaphore = false;
        uint8_t pad0[3] = {0, 0, 0};
        float speed = 1.2F;
        uint16_t waypointLinks[4]{0xFFFF};
        RoadDirectionLink directionLinks[4];
    };

    struct RoadWaypoint {
        S_vector pos = {0, 0, 0};
        float speed = 1.2f;
        uint16_t prevWaypoint = 0xFFFF;
        uint16_t nextWaypoint = 0xFFFF;
        uint16_t prevCrosspoint = 0xFFFF;
        uint16_t nextCrosspoint = 0xFFFF;
    };

    enum WebNodeType : uint16_t {
        WPT_Pedestrian = 0x1,
        WPT_AI = 0x2,
        WPT_Traffic = 0x4,
        WPT_TramStation = 0x8,
        WPT_Special = 0x10,
        WPT_RailwayOnboard = 0x201,
        WPT_RailwayWaypoint = 0x1004,
        WPT_RailwayStop = 0x1008,
        WPT_TramUnknown = 0x1804,
        WPT_AIUnknown = 0x8002
    };

    enum WebConnectionType : uint16_t {
        WCT_Pedestrian = 0x1,
        WCT_AI = 0x2,
        WCT_TrafficForward = 0x4,
        WCT_Railway = 0x84,
        WCT_TrafficBackward = 0x8400,
        WCT_Other = 0x1000
    };

    struct WebConnection {
        uint16_t endPoint;
        WebConnectionType type;
        float length;
    };
#pragma pack(pop)

    struct WebNode {
        S_vector pos;
        WebNodeType type;
        uint16_t id;
        uint16_t radius;
        uint8_t unk[10];
        uint8_t numEnterLinks;
        uint8_t numExitLinks;

        std::vector<WebConnection> links;
    };

    static SceneEditor* Get() {
        static SceneEditor editor;

        return &editor;
    }

    bool Init();
    void Update();
    void Shutdown();

    bool IsInit() const;

    bool IsOpened() const { return !m_MissionPath.empty(); }

    bool IsShowingLightmapDialog() const { return m_ShowLightmapDialog; }

    std::string GetMissionPath() const { return m_MissionPath; }

    ScriptEditor* GetScriptEditor() { return &m_ScriptEditor; }

    void ShowLightmapDialog() { m_ShowLightmapDialog = true; }

    bool ToggleWebView() {
        m_DrawWebNodes = !m_DrawWebNodes;

        UINT state = m_DrawWebNodes ? MF_CHECKED : MF_UNCHECKED;

        CheckMenuItem(m_ViewMenu, MCMD_VIEW_WEBNODES, MF_BYCOMMAND | state);

        return m_DrawWebNodes;
    }

    bool ToggleRoadView() {
        m_DrawRoadPoints = !m_DrawRoadPoints;

        UINT state = m_DrawRoadPoints ? MF_CHECKED : MF_UNCHECKED;

        CheckMenuItem(m_ViewMenu, MCMD_VIEW_ROADPOINTS, MF_BYCOMMAND | state);

        return m_DrawRoadPoints;
    }

    bool ToggleCollisionView() {
        m_DrawCollisions = !m_DrawCollisions;

        UINT state = m_DrawCollisions ? MF_CHECKED : MF_UNCHECKED;

        CheckMenuItem(m_ViewMenu, MCMD_VIEW_COLLISIONS, MF_BYCOMMAND | state);

        return m_DrawCollisions;
    }

    HWND GetWindowHandle() const { return m_IGraph->GetMainHWND(); }

    ImGuiContext* GetImGuiContext() const { return m_ImGuiContext; }

    void SetLoadProgress(float progress) { m_LoadPercentage = progress; }
    void DrawProgress(const std::string& title = "Loading scene, please wait...");

    void MoveCameraToTarget(const S_vector& pos) {
        m_IsMovingTowardsTarget = true;
        auto dir = m_Camera->GetDir();
        dir *= 3.5f;
        m_TargetPos = pos - dir;
    }

    void SelectFrame(I3D_frame* frame) {
        if(m_SelectedFrame && strcmp(m_SelectedFrameName, m_SelectedFrame->m_pSzName) != 0) {
            auto entry = FindEntryByName(m_SelectedFrame->m_pSzName);

            if(entry) { entry->name = m_SelectedFrameName; }

            m_SelectedFrame->SetName(m_SelectedFrameName);
        }

        ClearSelection();

        m_SelectedFrame = frame;

        m_SelectionType = SEL_FRAME;

        if(m_SelectedFrame) {
            strcpy(m_SelectedFrameName, frame->m_pSzName);
            strcpy(m_SelectedFrameModel, frame->m_pSzModelName);
            m_SelectedFramePos = frame->GetPos();
            m_SelectedFrameWorldPos = frame->GetWorldPos();
            m_SelectedFrameEuler = EulerFromQuat(frame->GetRot());
            m_SelectedFrameScale = frame->GetScale();

            m_SelectedActor = GetActor(frame);

            m_ShowTransformOptions = true;
        }
    }

    void SelectCrosspoint(RoadCrosspoint* cross) {
        m_SelectionType = SEL_CROSSPOINT;
        m_SelectedCrosspoint = cross;

        m_ShowTransformOptions = true;
    }

    void SelectWaypoint(RoadWaypoint* point) {
        m_SelectionType = SEL_WAYPOINT;
        m_SelectedWaypoint = point;

        m_ShowTransformOptions = true;
    }

    void SelectWebNode(WebNode* node) {
        m_SelectionType = SEL_NODE;
        m_SelectedNode = node;

        m_ShowTransformOptions = true;
    }

    void SelectScript(GameScript* script) {
        m_SelectionType = SEL_SCRIPT;
        m_SelectedScript = script;
    }

    I3D_frame* GetSelectedFrame() const { return m_SelectedFrame; }
    RoadCrosspoint* GetSelectedCrosspoint() const { return m_SelectedCrosspoint; }
    RoadWaypoint* GetSelectedWaypoint() const { return m_SelectedWaypoint; }
    WebNode* GetSelectedWebNode() const { return m_SelectedNode; }
    GameScript* GetSelectedScript() const { return m_SelectedScript; }

    I3D_dummy* CreateDummy(bool inFrontOfCamera = false);
    I3D_sound* CreateSound(const std::string& soundName, bool inFrontOfCamera = false);
    I3D_light* CreateLight(bool inFrontOfCamera = false);
    I3D_model* CreateModel(const std::string& modelName, bool inFrontOfCamera = false);

    RoadCrosspoint* CreateCrosspoint();
    RoadWaypoint* CreateWaypoint();
    WebNode* CreateWebNode();
    GameScript* CreateScript();

    void IterateFrames(I3D_frame* frame, I3D_FRAME_TYPE type);

    void ClearSelection() {
        m_SelectionType = SEL_NONE;
        m_SelectedActor = nullptr;
        m_SelectedFrame = nullptr;
        m_SelectedWaypoint = nullptr;
        m_SelectedWaypointID = 0xFFFF;
        m_SelectedCrosspoint = nullptr;
        m_SelectedCrosspointID = 0xFFFF;
        m_SelectedNode = nullptr;
        m_SelectedNodeID = 0xFFFF;
        m_SelectedCityPart = nullptr;
        m_SelectedScript = nullptr;

        m_ShowTransformOptions = false;
    }

    void CopySelection();

    void PasteSelection();

    void CopyFrame(I3D_frame* frame) {
        m_CopySelectionType = SEL_FRAME;
        m_CopiedObject = frame;
    }

    void PasteFrame();

    void DeleteSelection();

    void DeleteFrame(I3D_frame* frame);

    Actor* GetActor(I3D_frame* frame);
    Actor* CreateActor(ActorType type, I3D_frame* frame);
    void DeleteActor(I3D_frame* frame);

    void ShowPopupMessage(const std::string& msg, float duration = 5.0f, float fadeOutAt = 2.0f) {
        m_DrawPopupMessage = true;
        m_PopupMessageTime = 0;
        m_PopupMessageDuration = duration;
        m_PopupMessageFadeOutStart = fadeOutAt;
        m_PopupMessage = msg;
    }

    bool Load(const std::string& path);
    void Save(const std::string& path);

    void ToggleInput(bool state);

    void Clear();

  private:
    struct CityPart {
        std::string name;
        I3D_frame* frame;
        I3D_bbox bbox;
        S_vector2 pos;
        float radius;
        S_vector2 leftBottomCorner;
        S_vector2 leftSide;
        S_vector2 rightBottomCorner;
        S_vector2 rightSide;
        S_vector2 leftTopCorner;
        S_vector2 bottomSide;
        S_vector2 rightTopCorner;
        S_vector2 topSide;
        std::vector<I3D_model*> models{};
    };

    struct CollisionVolume {
        enum VolumeType : uint8_t {
            VOLUME_FACE0 = 0,
            VOLUME_FACE1 = 1,
            VOLUME_FACE2 = 2,
            VOLUME_FACE3 = 3,
            VOLUME_FACE4 = 4,
            VOLUME_FACE5 = 5,
            VOLUME_FACE6 = 6,
            VOLUME_FACE7 = 7,

            VOLUME_XTOBB = 0x80,
            VOLUME_AABB = 0x81,
            VOLUME_SPHERE = 0x82,
            VOLUME_OBB = 0x83,
            VOLUME_CYLINDER = 0x84
        } type;

        std::string GetTypeAsString() {
            switch(type) {
            case VOLUME_FACE0: return "Face 0";
            case VOLUME_FACE1: return "Face 1";
            case VOLUME_FACE2: return "Face 2";
            case VOLUME_FACE3: return "Face 3";
            case VOLUME_FACE4: return "Face 4";
            case VOLUME_FACE5: return "Face 5";
            case VOLUME_FACE6: return "Face 6";
            case VOLUME_FACE7: return "Face 7";
            case VOLUME_XTOBB: return "XTOBB";
            case VOLUME_AABB: return "AABB";
            case VOLUME_SPHERE: return "Sphere";
            case VOLUME_OBB: return "OBB";
            case VOLUME_CYLINDER: return "Cylinder";
            }

            return "";
        }

        uint8_t sortInfo;
        uint8_t flags;
        uint8_t mtlId;
        I3D_frame* linkedFrame = nullptr;
        uint32_t linkType = 0;

        void ReadData(BinaryReader* reader) {
            type = static_cast<CollisionVolume::VolumeType>(reader->ReadUInt8());
            sortInfo = reader->ReadUInt8();
            flags = reader->ReadUInt8();
            mtlId = reader->ReadUInt8();
        }
        void WriteData(BinaryWriter* writer) {
            writer->WriteUInt8(static_cast<uint8_t>(type));
            writer->WriteUInt8(sortInfo);
            writer->WriteUInt8(flags);
            writer->WriteUInt8(mtlId);
        }
    };

    struct CollisionTriangle : public CollisionVolume {
        struct VertexLink {
            uint16_t vertexBufferIndex = 0xFFFF;
            S_vector vertexPos{0, 0, 0};
            I3D_frame* linkedFrame = nullptr;
        } vertices[3];

        S_vector CalculateCentroid() { return (vertices[0].vertexPos + vertices[1].vertexPos + vertices[2].vertexPos) * (1.0f / 3.0f); }

        S_plane plane;
    };

    struct CollisionXTOBB : public CollisionVolume {
        S_vector min, max, minExtent, maxExtent;
        S_matrix transform, inverseTransform;
    };

    struct CollisionAABB : public CollisionVolume {
        S_vector min, max;
    };

    struct CollisionSphere : public CollisionVolume {
        S_vector pos;
        float radius;
    };

    struct CollisionOBB : public CollisionVolume {
        S_vector minExtent, maxExtent;

        S_matrix transform, inverseTransform;
    };

    struct CollisionCylinder : public CollisionVolume {
        S_vector2 pos;
        float radius;
    };

    struct CollisionLink {
        uint32_t type;
        I3D_frame* frame;
    };

    struct CollisionCell {
        uint32_t numReferences;
        int unk;
        float height;
        int unk2;

        struct Reference {
            int volumeBufferOffset;
            int volumeType;
        };
        std::vector<Reference> references;
        std::vector<uint8_t> flags;
    };

    struct CollisionGrid {
        S_vector2 min;
        S_vector2 max;
        S_vector2 cellSize;
        uint32_t width;
        uint32_t length;
        float unk;
        S_vector unk2;
        uint32_t numFaces;
        uint32_t numXTOBBs;
        uint32_t numAABBs;
        uint32_t numSpheres;
        uint32_t numOBBs;
        uint32_t numCylinders;
        struct Bounds {
            std::vector<float> x;
            std::vector<float> y;
        } bounds;
    };

    struct CollisionHeader {
        uint32_t version;
        uint32_t gridDataOffset;
        uint32_t numLinks;
    };

    struct CollisionManager {
        void Clear() {
            ZeroMemory(&header, sizeof(CollisionHeader));

            unk1 = unk2 = 0;

            links.clear();

            grid.bounds.x.clear();
            grid.bounds.y.clear();
            ZeroMemory(&grid, sizeof(CollisionGrid) - sizeof(CollisionGrid::Bounds));

            tris.clear();
            aabbs.clear();
            xtobbs.clear();
            cylinders.clear();
            obbs.clear();
            spheres.clear();

            cells.clear();
        }

        CollisionHeader header;
        std::vector<CollisionLink> links;
        CollisionGrid grid;

        uint32_t unk1, unk2;

        std::vector<CollisionVolume*> volumes;

        std::vector<CollisionTriangle> tris;
        std::vector<CollisionAABB> aabbs;
        std::vector<CollisionXTOBB> xtobbs;
        std::vector<CollisionCylinder> cylinders;
        std::vector<CollisionOBB> obbs;
        std::vector<CollisionSphere> spheres;

        std::vector<CollisionCell> cells;
    };

    struct HierarchyEntry {
        std::string name = "";
        I3D_frame* frame = nullptr;
        Actor* actor = nullptr;
        HierarchyEntry* parent = nullptr;
        std::vector<CollisionVolume*> collisions;
        std::vector<HierarchyEntry> children;
    };

    S_vector SnapToGrid(const S_vector& pos) {
        const float gridSize = m_GridScale; // Same as grid step size
        S_vector snapped = pos;
        snapped.x = roundf(pos.x / gridSize) * gridSize; // Snap X to nearest grid point
        snapped.y = roundf(pos.y / gridSize) * gridSize; // Snap X to nearest grid point
        snapped.z = roundf(pos.z / gridSize) * gridSize; // Snap Z to nearest grid point
        return snapped;
    }

    S_vector SnapToGridRotated(const S_vector& pos, const S_matrix& rotationMatrix) {
        const float gridSize = m_GridScale; // Same as grid step size
        // Transform position to local space (inverse of rotation)
        S_matrix invRotation = rotationMatrix;
        invRotation.Inverse(rotationMatrix); // Compute inverse
        S_vector localPos = pos.RotateByMatrix(invRotation);
        // Snap X, Y and Z in local space
        S_vector snapped = localPos;
        snapped.x = roundf(localPos.x / gridSize) * gridSize;
        snapped.y = roundf(localPos.y / gridSize) * gridSize;
        snapped.z = roundf(localPos.z / gridSize) * gridSize;
        // Transform back to world space
        return snapped.RotateByMatrix(rotationMatrix);
    }

    bool IsMouseDeltaSignificant(float threshold = 1e-3f) {
        ImVec2 mouseDelta = ImGui::GetIO().MouseDelta; // Get mouse delta (x, y)
        return (fabs(mouseDelta.x) > threshold || fabs(mouseDelta.y) > threshold);
    }

    bool IsDescendant(I3D_frame* frame, I3D_frame* targetFrame);

    void UpdateHierarchyAfterDragDrop(I3D_frame* draggedFrame, I3D_frame* targetFrame, bool reordering = false, int dropIndex = -1);

    void ShowFrameEntry(const HierarchyEntry& entry);

    I3D_frame* FindFrameByName(const std::string& name) {
        // Search through each entry in the root vector
        for(HierarchyEntry& entry: m_Hierarchy) {
            // Check if current entry matches the target name
            if(entry.name == name) { return entry.frame; }

            // Recursively search through children of current entry
            for(HierarchyEntry& child: entry.children) {
                HierarchyEntry* result = FindEntryByName(child, name);
                if(result != nullptr) { return result->frame; }
            }
        }

        // If not found, return nullptr
        return nullptr;
    }

    HierarchyEntry* FindEntryByFrame(I3D_frame* frame) {
        // Search through each entry in the root vector
        for(HierarchyEntry& entry: m_Hierarchy) {
            // Check if current entry matches the target name
            if(entry.frame == frame) { return &entry; }

            // Recursively search through children of current entry
            for(HierarchyEntry& child: entry.children) {
                HierarchyEntry* result = FindEntryByFrame(child, frame);
                if(result != nullptr) { return result; }
            }
        }

        // If not found, return nullptr
        return nullptr;
    }

    HierarchyEntry* FindEntryByName(const std::string& name) {
        // Search through each entry in the root vector
        for(HierarchyEntry& entry: m_Hierarchy) {
            // Check if current entry matches the target name
            if(entry.name == name) { return &entry; }

            // Recursively search through children of current entry
            for(HierarchyEntry& child: entry.children) {
                HierarchyEntry* result = FindEntryByName(child, name);
                if(result != nullptr) { return result; }
            }
        }

        // If not found, return nullptr
        return nullptr;
    }

    HierarchyEntry* FindEntryByFrame(HierarchyEntry& node, I3D_frame* frame) {
        // Check if current node matches the target name
        if(node.frame == frame) { return &node; }

        // Recursively search through children
        for(HierarchyEntry& child: node.children) {
            HierarchyEntry* result = FindEntryByFrame(child, frame);
            if(result != nullptr) { return result; }
        }

        return nullptr;
    }

    HierarchyEntry* FindEntryByName(HierarchyEntry& node, const std::string& name) {
        // Check if current node matches the target name
        if(node.name == name) { return &node; }

        // Recursively search through children
        for(HierarchyEntry& child: node.children) {
            HierarchyEntry* result = FindEntryByName(child, name);
            if(result != nullptr) { return result; }
        }

        return nullptr;
    }

    bool AreDirectionsOpposite(const S_vector& v1, const S_vector& v2) {
        S_vector v1Norm = v1;
        v1Norm.SetNormalized(v1Norm);
        S_vector v2Norm = v2;
        v2Norm.SetNormalized(v2Norm);

        float dot = v1Norm.Dot(v2Norm);
        return dot < MRG_ZERO;
    }

    S_vector CalculateDirection(const S_vector& start, const S_vector& end, bool normalize = true) {
        // Subtract start from end to get direction vector
        S_vector direction = end - start;

        // Normalize if requested
        if(normalize) { direction.SetNormalized(direction); }

        return direction;
    }

    S_vector CalculateRightDirection(const S_vector& forward) {
        S_vector forwardNorm = forward;
        forwardNorm.SetNormalized(forwardNorm);

        S_vector forwardXZ = S_vector(forwardNorm.x, 0.0f, forwardNorm.z);
        forwardXZ.SetNormalized(forwardXZ);

        if(forwardXZ.Magnitude2() < MRG_ZERO) { forwardXZ = S_vector(0.0f, 0.0f, 1.0f); }

        S_vector up = S_vector(0.0f, 1.0f, 0.0f);

        S_vector right = up.Cross(forwardXZ);
        right.SetNormalized(right);

        return right;
    }

    S_vector CalculateWaypointForwardDirection(const RoadWaypoint& waypoint) {
        S_vector forward = S_vector(1.0f, 0.0f, 0.0f);

        S_vector currentPos = waypoint.pos;

        S_vector prevPos;
        bool hasPrev = false;
        uint16_t prevIndex = waypoint.prevWaypoint & 0x7FFF;
        if(waypoint.prevWaypoint != 0xFFFF) {
            if(waypoint.prevWaypoint & 0x8000) {
                if(prevIndex < m_RoadWaypoints.size()) {
                    prevPos = m_RoadWaypoints[prevIndex].pos;
                    hasPrev = true;
                }
            } else { // Crosspoint
                if(prevIndex < m_RoadCrosspoints.size()) {
                    prevPos = m_RoadCrosspoints[waypoint.prevWaypoint].pos;
                    hasPrev = true;
                }
            }
        }

        S_vector nextPos;
        bool hasNext = false;
        uint16_t nextIndex = waypoint.nextWaypoint & 0x7FFF;
        if(waypoint.nextWaypoint != 0xFFFF) {
            if(waypoint.nextWaypoint & 0x8000) {
                if(nextIndex < m_RoadWaypoints.size()) {
                    nextPos = m_RoadWaypoints[nextIndex].pos;
                    hasNext = true;
                }
            } else { // Crosspoint
                if(nextIndex < m_RoadCrosspoints.size()) {
                    nextPos = m_RoadCrosspoints[waypoint.nextWaypoint].pos;
                    hasNext = true;
                }
            }
        }

        if(hasPrev && hasNext) {
            forward = nextPos - prevPos;
            forward.SetNormalized(forward);

            if(forward.Magnitude2() < MRG_ZERO) { forward = S_vector(1.0f, 0.0f, 0.0f); }
        } else if(hasNext) {
            forward = nextPos - currentPos;
            forward.SetNormalized(forward);
            if(forward.Magnitude2() < MRG_ZERO) { forward = S_vector(1.0f, 0.0f, 0.0f); }
        } else if(hasPrev) {
            forward = currentPos - prevPos;
            forward.SetNormalized(forward);
            if(forward.Magnitude2() < MRG_ZERO) { forward = S_vector(1.0f, 0.0f, 0.0f); }
        }

        return forward;
    }

    void BuildHierarchyRecursively(I3D_frame* frame, HierarchyEntry* parentEntry) {
        if(!frame) { return; }

        // Iterate through all children of the current frame
        for(uint32_t i = 0; i < frame->NumChildren(); ++i) {
            I3D_frame* childFrame = frame->GetChild(i);
            if(childFrame) {
                // Create a new HierarchyEntry for the child
                auto& childEntry = parentEntry->children.emplace_back();
                childEntry.name = childFrame->GetName() ? childFrame->GetName() : "Unnamed";
                childEntry.frame = childFrame;
                childEntry.actor = GetActor(childFrame); // Assuming no actor is associated; adjust if needed
                childEntry.parent = parentEntry;
                childEntry.children = {};

                // Recursively process the child's children
                BuildHierarchyRecursively(childFrame, &childEntry);
            }
        }
    }

    std::vector<HierarchyEntry> BuildSceneTree() {
        std::vector<HierarchyEntry> hierarchy;

        // Add Primary sector as root
        auto& primaryEntry = hierarchy.emplace_back();
        primaryEntry.name = "Primary sector";
        primaryEntry.frame = m_PrimarySector;
        primaryEntry.actor = nullptr;
        primaryEntry.parent = nullptr;
        primaryEntry.children = {};

        // Add Backdrop sector as root
        auto& backEntry = hierarchy.emplace_back();
        backEntry.name = "Backdrop sector";
        backEntry.frame = m_BackdropSector;
        backEntry.actor = nullptr;
        backEntry.parent = nullptr;
        backEntry.children = {};

        // Recursively build the hierarchy for Primary sector
        if(m_PrimarySector) { BuildHierarchyRecursively(m_PrimarySector, &hierarchy[0]); }

        // Recursively build the hierarchy for Backdrop sector
        if(m_BackdropSector) { BuildHierarchyRecursively(m_BackdropSector, &hierarchy[1]); }

        return hierarchy;
    }

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

    bool LoadSceneBin(const std::string& fileName);
    bool LoadCacheBin(const std::string& fileName);
    bool LoadRoadBin(const std::string& fileName);
    bool LoadCheckBin(const std::string& fileName);
    bool LoadTreeKlz(const std::string& fileName);
    bool ReadChunk(E_CHUNK_TYPE chunkType, ChunkReader& chunk);
    void WriteSceneBin(const std::string& fileName);
    void WriteCacheBin(const std::string& fileName);
    void WriteRoadBin(const std::string& fileName);
    void WriteCheckBin(const std::string& fileName);
    void WriteTreeKlz(const std::string& fileName);

    void WriteFrameRecursively(I3D_frame* frame, ChunkWriter& chunk, bool writeParent = false);
    void WriteFrame(I3D_frame* frame, ChunkWriter& chunk, bool forceWrite = false);

    const char* GetWebNodeTypeName(WebNodeType eType) {
        switch(eType) {
        case WPT_Pedestrian: return "Pedestrian";
        case WPT_AI: return "AI";
        case WPT_Traffic: return "Traffic";
        case WPT_TramStation: return "TramStation";
        case WPT_Special: return "Special";
        case WPT_RailwayOnboard: return "Railway Onboard";
        case WPT_RailwayWaypoint: return "Railway Waypoint";
        case WPT_RailwayStop: return "Railway Stop";
        case WPT_TramUnknown: return "Tram Unknown";
        case WPT_AIUnknown: return "AI Unknown";
        default: return "Unknown";
        }
    }

    S_vector GetWebNodeTypeColor(WebNodeType eType) {
        switch(eType) {
        case WPT_Pedestrian: return S_vector(0, 0, 1);
        case WPT_AI: return S_vector(0, 1, 0);
        case WPT_Traffic: return S_vector(1, 1, 0);
        case WPT_TramStation: return S_vector(1, 0, 0);
        case WPT_Special: return S_vector(0.59f, 0, 1);
        case WPT_RailwayOnboard: return S_vector(1, 0.5f, 0.5f);
        case WPT_RailwayWaypoint: return S_vector(0.75f, 0.75f, 0);
        case WPT_RailwayStop: return S_vector(0.65f, 0, 0);
        case WPT_TramUnknown: return S_vector(0.85f, 0.25f, 0.25f);
        case WPT_AIUnknown: return S_vector(0.75f, 0, 0);
        default: return S_vector(0.65f, 0.65f, 0.65f);
        }
    }

    const char* GetWebConnTypeName(WebConnectionType eType) {
        switch(eType) {
        case WCT_Pedestrian: return "Pedestrian";
        case WCT_AI: return "AI";
        case WCT_TrafficForward: return "Traffic Forward";
        case WCT_TrafficBackward: return "Traffic Backward";
        case WCT_Railway: return "Railway";
        case WCT_Other: return "Other";
        default: return "Unknown";
        }
    }

    S_vector GetWebConnTypeColor(WebConnectionType eType) {
        switch(eType) {
        case WCT_Pedestrian: return S_vector(0, 0, 1);
        case WCT_AI: return S_vector(0, 1, 0);
        case WCT_TrafficForward: return S_vector(1, 1, 0);
        case WCT_TrafficBackward: return S_vector(0, 1, 1);
        case WCT_Railway: return S_vector(0.75f, 0.75f, 0);
        case WCT_Other: return S_vector(0.59f, 0, 1);
        default: return S_vector(0.45f, 0.45f, 0.45f);
        }
    }

    enum SelectionType { SEL_NONE, SEL_FRAME, SEL_PART, SEL_WAYPOINT, SEL_CROSSPOINT, SEL_NODE, SEL_SCRIPT };

    struct Modification {
        enum ModificationState { MDFS_CREATE, MDFS_MODIFY, MDFS_DELETE };

        Modification(const std::string& desc, ModificationState state, void* data, size_t size, void* userData, SelectionType userDataType) {
            this->description = desc;
            if(size && data) {
                this->payloadData = malloc(size);
                if(this->payloadData) {
                    memcpy(this->payloadData, data, size);
                    this->payloadSize = size;
                    this->payloadUserData = userData;
                    this->state = state;
                    this->userDataType = userDataType;
                }
            }
        }
        ~Modification() {
            if(this->payloadData) {
                free(this->payloadData);
                this->payloadData = nullptr;
            }
        }

        std::string description;
        ModificationState state;
        SelectionType userDataType;
        size_t payloadSize;
        void* payloadData;
        void* payloadUserData;
    };

    IGraph* m_IGraph = nullptr;
    I3D_driver* m_3DDriver = nullptr;
    ISND_driver* m_SoundDriver = nullptr;

    I3D_scene* m_Scene = nullptr;
    I3D_camera* m_Camera = nullptr;

    I3D_sector* m_PrimarySector = nullptr;
    I3D_sector* m_BackdropSector = nullptr;

    ITexture* m_SoundIconTexture = nullptr;
    ITexture* m_LightIconTexture = nullptr;
    ITexture* m_CameraIconTexture = nullptr;
    ITexture* m_DetectorIconTexture = nullptr;
    ITexture* m_TrafficIconTexture = nullptr;
    ITexture* m_PedsIconTexture = nullptr;

    I3D_material* m_SoundIconMaterial = nullptr;
    I3D_material* m_LightIconMaterial = nullptr;
    I3D_material* m_CameraIconMaterial = nullptr;
    I3D_material* m_DetectorIconMaterial = nullptr;
    I3D_material* m_TrafficIconMaterial = nullptr;
    I3D_material* m_PedsIconMaterial = nullptr;

    ImGuiContext* m_ImGuiContext = nullptr;

    IDirect3DDevice8* m_D3DDevice = nullptr;

    IDirectInputDevice8A* m_LS3DMouseDevice = nullptr;

    HMENU m_MenuBar = nullptr;
    HMENU m_FileMenu = nullptr;
    HMENU m_ViewMenu = nullptr;
    HMENU m_EditMenu = nullptr;
    HMENU m_CreateMenu = nullptr;
    HMENU m_CreateFrameMenu = nullptr;
    HMENU m_CreateActorMenu = nullptr;
    HMENU m_CreateRoadMenu = nullptr;
    HMENU m_HelpMenu = nullptr;

    SelectionType m_SelectionType = SEL_NONE;
    SelectionType m_CopySelectionType = SEL_NONE;
    I3D_frame* m_SelectedFrame = nullptr;
    Actor* m_SelectedActor = nullptr;
    CityPart* m_SelectedCityPart = nullptr;
    RoadWaypoint* m_SelectedWaypoint = nullptr;
    uint16_t m_SelectedWaypointID = 0;
    RoadCrosspoint* m_SelectedCrosspoint = nullptr;
    uint16_t m_SelectedCrosspointID = 0;
    WebNode* m_SelectedNode = nullptr;
    uint16_t m_SelectedNodeID = 0;
    GameScript* m_SelectedScript = nullptr;
    void* m_CopiedObject = nullptr;

    std::vector<CityPart> m_CacheParts{};
    std::map<uint16_t, RoadWaypoint> m_RoadWaypoints{};
    std::map<uint16_t, RoadCrosspoint> m_RoadCrosspoints{};
    std::map<uint16_t, WebNode> m_WebNodes{};

    std::string m_MissionPath = "";

    S_vector m_CameraPos = {0, 0, 0};
    S_vector m_CurCameraVelocity = {0, 0, 0};
    S_vector m_TargetCameraVelocity = {0, 0, 0};
    S_vector2 m_CameraRot = {0, 0};

    char m_SelectedFrameName[256]{0};
    char m_SelectedFrameModel[256]{0};
    S_vector m_SelectedFramePos = {0, 0, 0};
    S_vector m_SelectedFrameWorldPos = {0, 0, 0};
    S_vector m_SelectedFrameEuler = {0, 0, 0};
    S_vector m_SelectedFrameScale = {0, 0, 0};

    ImGuizmo::OPERATION m_CurrentTransformOperation = ImGuizmo::TRANSLATE;

    bool m_IsMovingTowardsTarget = false;
    S_vector m_TargetPos = {0, 0, 0};

    std::chrono::steady_clock::time_point m_LastFrameTime;
    float m_DeltaTime;

    float m_CameraFOV = 0;

    size_t m_CurFileSize = 0;
    size_t m_CurReadBytes = 0;
    float m_LoadPercentage = 0.0f;

    ScriptEditor m_ScriptEditor;

    std::vector<I3D_frame*> m_Frames;
    std::vector<I3D_frame*> m_CreatedFrames;
    std::vector<Actor*> m_Actors;
    std::vector<GameScript> m_Scripts;
    std::vector<I3D_lit_object*> m_LightmapObjects;
    std::vector<I3D_model*> m_AnimatedModels;
    std::vector<I3D_animation_set*> m_Anims;

    std::vector<HierarchyEntry> m_Hierarchy;

    CollisionManager m_ColManager;

    bool m_KeyboardEnabled = false;
    bool m_MouseEnabled = false;

    bool m_SceneLoaded = false;
    bool m_OpenFileDialog = false;

    bool m_DrawWebNodes = false;
    bool m_DrawRoadPoints = false;
    bool m_DrawCollisions = false;

    bool m_DrawPopupMessage = false;
    float m_PopupMessageTime = 0;
    float m_PopupMessageDuration = 5.0f;
    float m_PopupMessageFadeOutStart = 2.0f;
    std::string m_PopupMessage = "";

    uint16_t m_HighestCrosspointID = 0;
    uint16_t m_HighestWaypointID = 0;
    uint16_t m_HighestNodeID = 0;

    I3D_frame* m_HoveredFrame = nullptr;

    void* m_ReferencedData = nullptr;
    SelectionType m_ReferenceSelectionType = SEL_FRAME;
    I3D_FRAME_TYPE m_ReferencedFrameType = FRAME_NULL;
    int m_ReferencedSlotID = 0;
    bool m_SelectingCrosspoint = false;
    bool m_ShowingRefSelection = false;
    bool m_HasSelectedReference = false;

    std::vector<Modification> m_UndoModifications;
    std::vector<Modification> m_RedoModifications;

    enum WaypointSelectionType { WPS_NONE, WPS_NEXTWP, WPS_PREVWP, WPS_NEXTCP, WPS_PREVCP, WPS_WPLINK, WPS_CPLINK };
    WaypointSelectionType m_WaypointSelectionType = WPS_NONE;

    bool m_SavedProperly = false;
    bool m_ShowTransformOptions = false;
    bool m_ShowTransformGrid = false;
    bool m_TransformGridSnapping = false;
    float m_GridScale = 1.0f;
    int m_GridSize = 64;

    bool m_ShowLightmapDialog = false;
};