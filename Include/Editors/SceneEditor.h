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

#include <Config.h>

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
    MCMD_VIEW_CITYPARTS,
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

    // Create -> Collision menu
    MCMD_CREATE_AABB = 470,
    MCMD_CREATE_OBB,
    MCMD_CREATE_XTOBB,
    MCMD_CREATE_SPHERE,
    MCMD_CREATE_CYLINDER,
    MCMD_CREATE_MESH,

    // Create menu
    MCMD_CREATE_WEBNODE = 480,
    MCMD_CREATE_SCRIPT,

    // Window menu
    MCMD_WINDOW_SCENESETTINGS = 500,
    MCMD_WINDOW_COLLISIONSETTINGS,
    MCMD_WINDOW_LMG,

    // Help menu
    MCMD_HELP_ABOUT = 600
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

    bool IsShowingSceneSettings() const { return m_ShowingSceneSettings; }
    bool IsShowingCollisionSettings() const { return m_ShowingCollisionSettings; }

    bool IsShowingLightmapDialog() const { return m_ShowLightmapDialog; }

    std::string GetMissionPath() const { return m_MissionPath; }

    ScriptEditor* GetScriptEditor() { return &m_ScriptEditor; }

    I3D_scene* GetScene() const { return m_Scene; }
    I3D_camera* GetCamera() const { return m_Camera; }

    void ShowSceneSettings() { m_ShowingSceneSettings = true; }
    void ShowCollisionSettings() { m_ShowingCollisionSettings = true; }
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

    bool ToggleCityPartsView() {
        m_DrawCityParts = !m_DrawCityParts;

        UINT state = m_DrawCityParts ? MF_CHECKED : MF_UNCHECKED;

        CheckMenuItem(m_ViewMenu, MCMD_VIEW_CITYPARTS, MF_BYCOMMAND | state);

        return m_DrawCityParts;
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
    inline I3D_frame* FindModelAncestor(I3D_frame* frame) {
        while(frame != m_PrimarySector) {
            if(frame->GetType() == FRAME_MODEL) { return frame; }
            frame = frame->GetParent();
        }
        return frame; // Fallback: original (null/root)
    }

    struct CityPart {
        std::string name;
        I3D_frame* frame;
        I3D_bbox bbox;
        S_vector2 spherePos;
        float sphereRadius;
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

    struct Collider {
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

        bool IsMeshCollider() const {
            return type == VOLUME_FACE0 || type == VOLUME_FACE1 || type == VOLUME_FACE2 || type == VOLUME_FACE3 || type == VOLUME_FACE4 ||
                   type == VOLUME_FACE5 || type == VOLUME_FACE6 || type == VOLUME_FACE7;
        }

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

            return "Unknown";
        }

        uint8_t sortInfo;
        uint8_t flags;
        uint8_t mtlId;
        I3D_frame* linkedFrame = nullptr;

        void ReadData(BinaryReader* reader) {
            type = static_cast<Collider::VolumeType>(reader->ReadUInt8());
            sortInfo = reader->ReadUInt8();
            flags = reader->ReadUInt8();
            mtlId = reader->ReadUInt8();
        }
        void WriteData(BinaryWriter* writer) const {
            writer->WriteUInt8(static_cast<uint8_t>(type));
            writer->WriteUInt8(sortInfo);
            writer->WriteUInt8(flags);
            writer->WriteUInt8(mtlId);
        }
    };

    struct TriangleCollider : public Collider {
        struct VertexLink {
            uint16_t vertexBufferIndex = 0xFFFF;
            S_vector vertexPos{0, 0, 0};
            I3D_frame* linkedFrame = nullptr;
        } vertices[3];

        S_vector CalculateCentroid() { return (vertices[0].vertexPos + vertices[1].vertexPos + vertices[2].vertexPos) * (1.0f / 3.0f); }

        S_plane plane;
    };

    struct MeshCollider : public Collider {
        std::vector<TriangleCollider*> tris;
    };

    MeshCollider* GetMeshCollider(I3D_frame* frame) {
        for(MeshCollider* collider: m_ColManager.meshes) {
            if(collider->linkedFrame == frame) return collider;
        }

        return nullptr;
    }

    bool IsMeshColliderPresent(I3D_frame* frame) const {
        for(const MeshCollider* collider: m_ColManager.meshes) {
            if(collider->linkedFrame == frame) return true;
        }

        return false;
    }

    struct XTOBBCollider : public Collider {
        S_vector min, max, minExtent, maxExtent;
        S_matrix transform, inverseTransform;
    };

    struct AABBCollider : public Collider {
        S_vector min, max;
    };

    struct SphereCollider : public Collider {
        S_vector pos;
        float radius;
    };

    struct OBBCollider : public Collider {
        S_vector minExtent, maxExtent;

        S_matrix transform, inverseTransform;
    };

    struct CylinderCollider : public Collider {
        S_vector2 pos;
        float radius;
    };

    struct CollisionLink {
        enum LinkType { LINK_NONE, LINK_SURFACE, LINK_VOLUME } type;
        I3D_frame* frame;
    };

    struct GridCell {
        uint32_t numVolumes;
        float height;
        float width;

        struct Reference {
            int volumeBufferOffset;
            int volumeType;
        };
        std::vector<Reference> references;
        std::vector<uint8_t> flags;

        std::vector<TriangleCollider*> sortedColliders;
    };

    struct CollisionGrid {
        S_vector2 min{0, 0};
        S_vector2 max{0, 0};
        S_vector2 cellSize{5, 5};
        uint32_t width = 0;
        uint32_t length = 0;
        struct Bounds {
            std::vector<float> x;
            std::vector<float> y;
        } bounds;
    };

    struct ColliderInfo {
        size_t offset;
        uint8_t type;
    };

    struct CollisionManager {
        void Clear() {
            colliders.clear();

            links.clear();

            grid.bounds.x.clear();
            grid.bounds.y.clear();
            ZeroMemory(&grid, sizeof(CollisionGrid) - sizeof(CollisionGrid::Bounds));
            grid.cellSize = {5, 5};

            for(TriangleCollider* collider: tris) {
                delete collider;
            }
            tris.clear();

            for(AABBCollider* collider: aabbs) {
                delete collider;
            }
            aabbs.clear();

            for(XTOBBCollider* collider: xtobbs) {
                delete collider;
            }
            xtobbs.clear();

            for(CylinderCollider* collider: cylinders) {
                delete collider;
            }
            cylinders.clear();

            for(OBBCollider* collider: obbs) {
                delete collider;
            }
            obbs.clear();

            for(SphereCollider* collider: spheres) {
                delete collider;
            }
            spheres.clear();

            for(MeshCollider* collider: meshes) {
                delete collider;
            }
            meshes.clear();

            cells.clear();
        }

        void GenerateGrid() {
            auto UpdateBounds = [&](const S_vector2& pos) {
                grid.min.x = min(grid.min.x, pos.x);
                grid.min.y = min(grid.min.y, pos.y);
                grid.max.y = max(grid.max.y, pos.y);
                grid.max.y = max(grid.max.y, pos.y);
            };

            for(const MeshCollider* collider: meshes) {
                S_matrix worldMat = collider->linkedFrame->GetWorldMat();

                for(const TriangleCollider* tri: collider->tris) {
                    for(int i = 0; i < 3; i++) {
                        const S_vector pos = tri->vertices[i].vertexPos * worldMat;
                        UpdateBounds({pos.x, pos.z});
                    }
                }
            }

            for(const AABBCollider* collider: aabbs) {
                UpdateBounds({collider->min.x, collider->min.z});
                UpdateBounds({collider->max.x, collider->max.z});
            }

            for(const XTOBBCollider* collider: xtobbs) {
                UpdateBounds({collider->min.x, collider->min.z});
                UpdateBounds({collider->max.x, collider->max.z});
            }

            for(const CylinderCollider* collider: cylinders) {
                UpdateBounds({collider->pos.x - collider->radius, collider->pos.y - collider->radius});
                UpdateBounds({collider->pos.x + collider->radius, collider->pos.y + collider->radius});
            }

            for(const OBBCollider* collider: obbs) {
                S_vector minPos = collider->minExtent * collider->transform;
                S_vector maxPos = collider->maxExtent * collider->transform;

                UpdateBounds({minPos.x, minPos.z});
                UpdateBounds({maxPos.x, maxPos.z});
            }

            for(const SphereCollider* collider: spheres) {
                UpdateBounds({collider->pos.x - collider->radius, collider->pos.z - collider->radius});
                UpdateBounds({collider->pos.x + collider->radius, collider->pos.z + collider->radius});
            }

            uint32_t gridWidth = static_cast<uint32_t>(std::ceil((grid.max.x - grid.min.x) / grid.cellSize.x));
            uint32_t gridLength = static_cast<uint32_t>(std::ceil((grid.max.y - grid.min.y) / grid.cellSize.y));

            grid.bounds.x.resize(gridWidth + 1);
            grid.bounds.y.resize(gridLength + 1);
            for(uint32_t i = 0; i <= gridWidth; i++)
                grid.bounds.x[i] = grid.min.x + i * grid.cellSize.x;
            for(uint32_t i = 0; i <= gridLength; i++)
                grid.bounds.y[i] = grid.min.y + i * grid.cellSize.y;
        }

        #undef min
        #undef max

        void GenerateCells(const std::vector<ColliderInfo>& volumeInfos) {
            struct CellBuilder {
                std::vector<GridCell::Reference> refs;
                std::vector<uint8_t> flags;
            };
            std::vector<CellBuilder> cellBuilders(grid.width * grid.length);

            auto WorldToCell = [&](float x, float y) -> std::pair<int32_t, int32_t> {
                int32_t cx = static_cast<int32_t>((x - grid.min.x) / grid.cellSize.x);
                int32_t cy = static_cast<int32_t>((y - grid.min.y) / grid.cellSize.y);
                cx = std::clamp(cx, 0, static_cast<int32_t>(grid.width - 1));
                cy = std::clamp(cy, 0, static_cast<int32_t>(grid.length - 1));
                return {cx, cy};
            };

            // ---- assign faces ------------------------------------------------------
            size_t volIdx = 0;
            size_t triIdx = 0;
            for(auto it = tris.begin(); it != tris.end(); ++it, ++triIdx, ++volIdx) {
                const TriangleCollider* t = *it;
                // approximate triangle AABB in WORLD space
                float tminX = FLT_MAX, tmaxX = -FLT_MAX, tminY = FLT_MAX, tmaxY = -FLT_MAX;
                for(int j = 0; j < 3; ++j) {
                    const S_vector& p = t->vertices[j].vertexPos * t->vertices[j].linkedFrame->GetWorldMat();
                    tminX = std::min(tminX, p.x);
                    tmaxX = std::max(tmaxX, p.x);
                    tminY = std::min(tminY, p.y);
                    tmaxY = std::max(tmaxY, p.y);
                }
                if(tminX > tmaxX || tminY > tmaxY) continue; // Degenerate, skip

                auto [c0x, c0y] = WorldToCell(tminX, tminY);
                auto [c1x, c1y] = WorldToCell(tmaxX, tmaxY);
                for(int32_t cx = c0x; cx <= c1x; ++cx)
                    for(int32_t cy = c0y; cy <= c1y; ++cy) {
                        size_t cellIdx = static_cast<size_t>(cy) * grid.width + cx;
                        CellBuilder& cb = cellBuilders[cellIdx];
                        cb.refs.push_back({static_cast<int32_t>(volumeInfos[volIdx].offset), volumeInfos[volIdx].type});
                        cb.flags.push_back(t->flags);
                    }
            }

            // ---- primitive volumes -------------------------------------------------
            auto AddPrimitive = [&](const auto* vol, uint8_t type) {
                float vminX = FLT_MAX, vmaxX = -FLT_MAX, vminY = FLT_MAX, vmaxY = -FLT_MAX;
                // Compute WORLD AABB for primitive (assume vol has worldMin/Max or transform)
                // Example for AABB (assume min/max are world)
                if constexpr(std::is_same_v<std::decay_t<decltype(*vol)>, AABBCollider>) {
                    vminX = vol->min.x;
                    vmaxX = vol->max.x;
                    vminY = vol->min.y;
                    vmaxY = vol->max.y;
                } else if constexpr(std::is_same_v<std::decay_t<decltype(*vol)>, XTOBBCollider>) {
                    vminX = vol->min.x;
                    vmaxX = vol->max.x;
                    vminY = vol->min.y;
                    vmaxY = vol->max.y;
                } else if constexpr(std::is_same_v<std::decay_t<decltype(*vol)>, CylinderCollider>) {
                    vminX = vol->pos.x - vol->radius;
                    vmaxX = vol->pos.x + vol->radius;
                    vminY = vol->pos.y - vol->radius;
                    vmaxY = vol->pos.y + vol->radius;
                } else if constexpr(std::is_same_v<std::decay_t<decltype(*vol)>, OBBCollider>) {
                    // For OBB, compute AABB from extents + transform (approx)
                    S_vector corners[8] = {/* compute 8 corners from minExtent/maxExtent * transform */};
                    for(int k = 0; k < 8; ++k) {
                        vminX = std::min(vminX, corners[k].x);
                        vmaxX = std::max(vmaxX, corners[k].x);
                        vminY = std::min(vminY, corners[k].y);
                        vmaxY = std::max(vmaxY, corners[k].y);
                    }
                } else if constexpr(std::is_same_v<std::decay_t<decltype(*vol)>, SphereCollider>) {
                    vminX = vol->pos.x - vol->radius;
                    vmaxX = vol->pos.x + vol->radius;
                    vminY = vol->pos.y - vol->radius;
                    vmaxY = vol->pos.y + vol->radius;
                }

                if(vminX > vmaxX || vminY > vmaxY) return; // Degenerate

                auto [c0x, c0y] = WorldToCell(vminX, vminY);
                auto [c1x, c1y] = WorldToCell(vmaxX, vmaxY);
                for(int32_t cx = c0x; cx <= c1x; ++cx)
                    for(int32_t cy = c0y; cy <= c1y; ++cy) {
                        size_t cellIdx = static_cast<size_t>(cy) * grid.width + cx;
                        CellBuilder& cb = cellBuilders[cellIdx];
                        cb.refs.push_back({static_cast<int32_t>(volumeInfos[volIdx].offset), type});
                        cb.flags.push_back(vol->flags);
                    }
                //++volIdx;
            };

            // AABBs
            for(auto it = aabbs.begin(); it != aabbs.end(); ++it, ++volIdx)
                AddPrimitive(*it, Collider::VOLUME_AABB);
            // XTOBBs
            for(auto it = xtobbs.begin(); it != xtobbs.end(); ++it, ++volIdx)
                AddPrimitive(*it, Collider::VOLUME_XTOBB);
            // Cylinders
            for(auto it = cylinders.begin(); it != cylinders.end(); ++it, ++volIdx)
                AddPrimitive(*it, Collider::VOLUME_CYLINDER);
            // OBBs
            for(auto it = obbs.begin(); it != obbs.end(); ++it, ++volIdx)
                AddPrimitive(*it, Collider::VOLUME_OBB);
            // Spheres
            for(auto it = spheres.begin(); it != spheres.end(); ++it, ++volIdx)
                AddPrimitive(*it, Collider::VOLUME_SPHERE);

            cells.clear();
            cells.reserve(cellBuilders.size());
            for(const CellBuilder& cb: cellBuilders) {
                GridCell gc;
                gc.numVolumes = static_cast<uint32_t>(cb.refs.size());
                gc.height = 0.0f;
                gc.width = 0.0f;
                gc.references = cb.refs;
                gc.flags = cb.flags;
                cells.push_back(gc);
            }

            constexpr size_t TRI_BYTE_SIZE = 32; // Confirm exact

            for(GridCell& cell: cells) {
                if(cell.references.empty()) continue;

                std::vector<std::pair<GridCell::Reference, size_t>> sortedRefs;
                sortedRefs.reserve(cell.references.size());
                for(size_t i = 0; i < cell.references.size(); ++i) {
                    sortedRefs.emplace_back(cell.references[i], i);
                }

                std::stable_sort(sortedRefs.begin(), sortedRefs.end(), [&](const auto& pa, const auto& pb) {
                    const GridCell::Reference& ra = pa.first;
                    const GridCell::Reference& rb = pb.first;

                    if(ra.volumeType >= 0x80 && rb.volumeType < 0x80) return false;
                    if(ra.volumeType < 0x80 && rb.volumeType >= 0x80) return true;
                    if(ra.volumeType >= 0x80 && rb.volumeType >= 0x80) return false;

                    size_t triIdxA = (ra.volumeBufferOffset - volumeInfos[0].offset) / TRI_BYTE_SIZE;
                    size_t triIdxB = (rb.volumeBufferOffset - volumeInfos[0].offset) / TRI_BYTE_SIZE;

                    auto itA = tris.begin();
                    std::advance(itA, triIdxA);
                    const TriangleCollider* ta = *itA;

                    auto itB = tris.begin();
                    std::advance(itB, triIdxB);
                    const TriangleCollider* tb = *itB;

                    S_vector v1, v2, v3, v4, v5, v6;

                    v1 = ta->vertices[0].vertexPos * ta->vertices[0].linkedFrame->GetWorldMat();
                    v2 = ta->vertices[1].vertexPos * ta->vertices[1].linkedFrame->GetWorldMat();
                    v3 = ta->vertices[2].vertexPos * ta->vertices[2].linkedFrame->GetWorldMat();
                    v4 = tb->vertices[0].vertexPos * tb->vertices[0].linkedFrame->GetWorldMat();
                    v5 = tb->vertices[1].vertexPos * tb->vertices[1].linkedFrame->GetWorldMat();
                    v6 = tb->vertices[2].vertexPos * tb->vertices[2].linkedFrame->GetWorldMat();

                    float minAX = std::min({ v1.x, v2.x, v3.x });
                    float minBX = std::min({v4.x, v5.x, v6.x});
                    return minAX < minBX;
                });

                std::vector<uint8_t> newFlags(sortedRefs.size());
                for(size_t i = 0; i < sortedRefs.size(); ++i) {
                    cell.references[i] = sortedRefs[i].first;
                    size_t origIdx = sortedRefs[i].second;
                    newFlags[i] = cell.flags[origIdx];
                }
                cell.flags = std::move(newFlags);
            }
        }

        std::vector<CollisionLink> links;
        CollisionGrid grid;

        std::vector<Collider*> colliders;
        std::vector<MeshCollider*> meshes;

        std::vector<TriangleCollider*> tris;
        std::vector<AABBCollider*> aabbs;
        std::vector<XTOBBCollider*> xtobbs;
        std::vector<CylinderCollider*> cylinders;
        std::vector<OBBCollider*> obbs;
        std::vector<SphereCollider*> spheres;

        std::vector<GridCell> cells;
    };

    struct HierarchyEntry {
        std::string name = "";
        I3D_frame* frame = nullptr;
        Actor* actor = nullptr;
        HierarchyEntry* parent = nullptr;
        std::vector<Collider*> colliders;
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

    struct LineVertex {
        float x, y, z; // Position
        DWORD color; // Color (0xAARRGGBB)
    };

    void DrawBatchedLines(const std::vector<LineVertex>& vertices,
                          const std::vector<WORD>& indices = {}, // Optional indices
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
    HMENU m_WindowMenu = nullptr;
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

    std::string m_MissionFileSignature = " - Mafia mission file - written using " PROJECT_NAME " v" PROJECT_VER " -";

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
    bool m_ShowingSceneSettings = false;
    bool m_ShowingCollisionSettings = false;

    bool m_DrawWebNodes = false;
    bool m_DrawRoadPoints = false;
    bool m_DrawCollisions = false;
    bool m_DrawCityParts = false;

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