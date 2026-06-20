#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <Editors/BaseEditor.h>
#include <I3D/I3D_math.h>
#include <I3D/I3D_model.h>
#include <I3D/I3D_animation_set.h>
#include <IO/ChgFile.h>
#include <ImSequencer.h>

#define REP_MAGIC 5681

// ============================================================================
// .rep file data structures
// ============================================================================

#pragma pack(push, 1)

struct RepHeader {
    uint32_t magicByte;
    uint32_t sizeOfAnimationBlocks;
    uint32_t sizeOfObjectDefinitionsSection;
    uint32_t countOfObjectDefinitionBlocks;
    uint32_t fixedCCSequence;
    uint32_t countOfCameraChunks;
    uint32_t countOfCameraFocusChunks;
    uint32_t sizeOfScriptEventsSequence;
    uint32_t sizeOfDialogSection;
    uint8_t  padding[60];
};
static_assert(sizeof(RepHeader) == 96, "RepHeader must be 96 bytes");

struct AnimationBlock {
    uint32_t animationID;
    char     animationName[48];
};
static_assert(sizeof(AnimationBlock) == 52, "AnimationBlock must be 52 bytes");

struct ObjectDefinitionBlock {
    char     name[36];
    char     nameTypeFlags[36];
    uint64_t chkData[2];
    uint32_t chkDataEmpty;
    uint32_t timeActivation;
    uint32_t definitionID;
    uint32_t size;
    uint32_t offset;
};
static_assert(sizeof(ObjectDefinitionBlock) == 108, "ObjectDefinitionBlock must be 108 bytes");

// Human frames are prefixed by a sub-header: {uint32_t frameA; uint32_t type;}
// The sizes below are for the payload AFTER the sub-header.
// Type 1 (stride 40): 8 header + 32 data
struct HumanFrameA {
    S_vector position;
    S_quat   rotation;
    uint32_t animationID;
};
static_assert(sizeof(HumanFrameA) == 32, "HumanFrameA must be 32 bytes");

// Type 2 (stride 44): 8 header + 36 data
struct HumanFrameB {
    S_vector position;
    S_quat   rotation;
    uint32_t animationID;
    uint32_t animationOffset;  // Lower 12 bits = animation speed parameter
};
static_assert(sizeof(HumanFrameB) == 36, "HumanFrameB must be 36 bytes");

struct CarFrame {
    uint32_t frameA;
    uint32_t type;
    S_vector position;
    S_quat   rotation;
    uint32_t unk1;
    uint32_t unk2;
};
static_assert(sizeof(CarFrame) == 44, "CarFrame must be 44 bytes");

struct ActorFrame {
    uint32_t frameA;
    uint32_t type;
    S_vector position;
    S_quat   rotation;
    uint8_t  unk[36];
};
static_assert(sizeof(ActorFrame) == 72, "ActorFrame must be 72 bytes");

struct CameraChunk {
    uint32_t frameA;       // Start time (ms)
    uint32_t frameB;       // End time (ms) / next keyframe time
    uint32_t unkType;      // Interpolation flags
    S_vector position;     // Camera position (P0)
    S_vector curve;        // Outgoing tangent (T0)
    S_vector subCurve;     // Incoming tangent (T1)
    float    roll;         // Camera roll angle
    float    FOV;          // Field of view
    float    easeIn;       // Ease-in for S-curve (0.0-1.0)
    float    easeOut;      // Ease-out for S-curve (0.0-1.0)
};
static_assert(sizeof(CameraChunk) == 64, "CameraChunk must be 64 bytes");

struct CameraFocusChunk {
    uint32_t frameA_Focus; // Start time (ms)
    uint32_t frameB_Focus; // End time (ms) / next keyframe time
    uint32_t unkType;      // Interpolation flags
    S_vector position;     // Focus target position (P0)
    S_vector curve;        // Outgoing tangent (T0)
    S_vector subCurve;     // Incoming tangent (T1)
    float    easeIn;       // Ease-in for S-curve (0.0-1.0)
    float    easeOut;      // Ease-out for S-curve (0.0-1.0)
};
static_assert(sizeof(CameraFocusChunk) == 56, "CameraFocusChunk must be 56 bytes");

struct ZatmizeData {
    uint32_t unk1;
    float    fadeA;
    float    fadeB;
    uint32_t unk2;
    uint8_t  data[16];
};
static_assert(sizeof(ZatmizeData) == 32, "ZatmizeData must be 32 bytes");

struct ScriptsData {
    uint32_t time;
    char     name[36];
};
static_assert(sizeof(ScriptsData) == 40, "ScriptsData must be 40 bytes");

struct WavData {
    uint32_t time;
    uint32_t state;
    char     name[32];
};
static_assert(sizeof(WavData) == 40, "WavData must be 40 bytes");

struct PreLastObject {
    uint32_t time;
    uint32_t charID;
    uint32_t stringID;
    char     name[24];
};
static_assert(sizeof(PreLastObject) == 36, "PreLastObject must be 36 bytes");

struct LastObject {
    uint32_t timeA;
    uint32_t timeB;
    char     name[32];
};
static_assert(sizeof(LastObject) == 40, "LastObject must be 40 bytes");

#pragma pack(pop)

// ============================================================================
// Object type identification
// ============================================================================

enum class RecordObjectType {
    Unknown,
    Car,
    Human,
    Actor
};

inline RecordObjectType GetObjectType(const ObjectDefinitionBlock& def) {
    if(def.chkData[0] == 188978561032ULL && def.chkData[1] == 240518168632ULL) return RecordObjectType::Car;
    if(def.chkData[0] == 171798691848ULL && def.chkData[1] == 240518168620ULL) return RecordObjectType::Human;
    if(def.chkData[0] == 395136991240ULL && def.chkData[1] == 0ULL) return RecordObjectType::Actor;
    return RecordObjectType::Unknown;
}

inline const char* GetObjectTypeName(RecordObjectType type) {
    switch(type) {
    case RecordObjectType::Car:   return "Car";
    case RecordObjectType::Human: return "Human";
    case RecordObjectType::Actor: return "Actor";
    default:                      return "Unknown";
    }
}

// ============================================================================
// CutsceneEditor
// ============================================================================

class CutsceneEditor : public BaseEditor {
public:
    static CutsceneEditor* Get() {
        static CutsceneEditor editor;
        return &editor;
    }

    // BaseEditor virtual overrides
    void OnUpdate() override;
    void OnRender3D() override;
    void OnRenderUI() override;
    void OnDockLayout(ImGuiID dockspaceId) override;
    void OnMenuSetup() override;
    void OnMenuCommand(WORD cmd) override;
    void OnFileOpen() override;
    void OnFileClose() override;
    const char* GetEditorName() override { return "Cutscene Editor"; }
    const char* GetIconPath() override { return "mEdit\\Images\\Icons\\SceneEditor.png"; }
    void OnSceneLoaded() override;
    void OnClear() override;

    enum PlaybackState {
        PLAYBACK_STOPPED,
        PLAYBACK_PLAYING,
        PLAYBACK_PAUSED
    };

private:
    bool LoadRecordFile(const std::string& repPath, const std::string& scenePath);
    bool LoadChgFileAndApply(const std::string& chgPath);
    void CleanupDiffFrames();

    void TickPlayback();
    void ApplyFrameData(uint32_t timeMs);
    void ApplyCameraData(uint32_t timeMs);
    void SeekToTime(uint32_t timeMs);

    // Parsed .rep data
    RepHeader m_RepHeader{};
    std::vector<AnimationBlock> m_AnimationBlocks;
    std::vector<ObjectDefinitionBlock> m_ObjectDefs;
    std::vector<uint8_t> m_FrameDataBlob;
    std::vector<CameraChunk> m_CameraChunks;
    std::vector<CameraFocusChunk> m_CameraFocusChunks;

    // Script events
    std::vector<ZatmizeData> m_FadeEvents;
    std::vector<ScriptsData> m_ScriptEvents;
    std::vector<WavData> m_WavEvents;

    // Dialog
    std::vector<PreLastObject> m_DialogEntries;
    std::vector<LastObject> m_LastObjects;

    // Playback
    PlaybackState m_PlaybackState = PLAYBACK_STOPPED;
    uint32_t m_CurrentTick = 0;
    uint32_t m_TotalTicks = 0;
    float m_PlaybackSpeed = 1.0f;
    float m_TickAccumulator = 0.0f;
    bool m_CameraFollow = true;

    // Per-object keyframe playback state
    struct ObjectPlaybackState {
        const uint8_t* pCurrent = nullptr;  // Current keyframe (being applied)
        const uint8_t* pNext = nullptr;     // Next keyframe (to advance to)
        const uint8_t* pStart = nullptr;    // Start of keyframe data (after 8-byte header)
        const uint8_t* pEnd = nullptr;      // End of data region
        uint32_t strides[4] = {};           // Stride lookup table (type -> byte size)
        uint32_t endTime = 0;               // End time in ms (from ObjDef.timeActivation)
        bool finished = false;
    };

    // Matched frames in the 3D scene
    struct MatchedObject {
        ObjectDefinitionBlock* def = nullptr;
        RecordObjectType type = RecordObjectType::Unknown;
        I3D_frame* frame = nullptr;
        ObjectPlaybackState playback;
        int currentAnimID = -1;
        I3D_animation_set* currentAnimSet = nullptr;
    };
    std::vector<MatchedObject> m_MatchedObjects;

    void InitObjectPlayback(MatchedObject& match);
    void ApplyAnimation(MatchedObject& match, uint32_t animID);

    // UI state
    int m_SelectedObject = -1;
    bool m_ShowObjects = true;
    bool m_ShowCamera = true;
    bool m_ShowEvents = true;
    bool m_DrawCameraPath = true;
    bool m_DrawFocusPath = true;
    bool m_DrawObjectMarkers = true;

    // Timeline sequencer
    struct TimelineSequence : public ImSequencer::SequenceInterface {
        CutsceneEditor* editor = nullptr;

        int GetFrameMin() const override { return 0; }
        int GetFrameMax() const override { return editor ? (int)editor->m_TotalTicks : 0; }
        int GetItemCount() const override { return editor ? (int)editor->m_ObjectDefs.size() : 0; }
        const char* GetItemLabel(int index) const override {
            if(!editor || index < 0 || index >= (int)editor->m_ObjectDefs.size()) return "";
            return editor->m_ObjectDefs[index].name;
        }
        void Get(int index, int** start, int** end, int* type, unsigned int* color) override;

        // Per-item start/end caches
        std::vector<int> itemStarts;
        std::vector<int> itemEnds;
    } m_Timeline;

    int m_TimelineCurrentFrame = 0;
    int m_TimelineFirstFrame = 0;
    bool m_TimelineExpanded = true;
    int m_TimelineSelectedEntry = -1;

    // Menus
    HMENU m_ViewMenu = nullptr;

    // .chg diff data
    ChgFile m_ChgFile;
    std::vector<I3D_frame*> m_DiffFrames;
};
