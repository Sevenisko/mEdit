#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <I3D/I3D_math.h>

// .chg chunk IDs
enum E_CHG_CHUNK : uint16_t {
    CHG_ROOT              = 1234,
    CHG_FRAME_DEFINITION  = 100,
    CHG_ACTOR_ON_NEW      = 200,
    CHG_ACTOR_ON_EXISTING = 400,
    CHG_SCRIPT            = 500
};

// Frame type constants (matches I3D_FRAME_TYPE values used in .chg)
enum E_CHG_FRAME_TYPE : uint32_t {
    CHG_FT_LIGHT = 2,
    CHG_FT_SOUND = 4,
    CHG_FT_DUMMY = 6,
    CHG_FT_MODEL = 9,
};

// ============================================================================
// Type-specific data for frame definitions
// ============================================================================

struct ChgLightData {
    uint32_t lightType = 0;
    S_vector color = {1, 1, 1};
    float range = 10.0f;
    uint8_t isOn = 1;
    float coneInner = 0.0f;
    float coneOuter = 0.0f;
    float falloffNear = 0.0f;
    float falloffFar = 0.0f;
    uint32_t lightMode = 0;
    std::vector<std::string> sectorLinks;
};

struct ChgSoundData {
    std::string soundFileName;
    float param0 = 0.0f;
    uint8_t isOn = 1;
    float soundParam1 = 0.0f;
    float soundParam2 = 0.0f;
    float soundParam3 = 0.0f;
    float soundParam4 = 0.0f;
    float rangeNear = 0.0f;
    float rangeFar = 10.0f;
    float coneAngle = 0.0f;
    float ambient = 0.0f;
    uint8_t loop = 0;
    float volume = 1.0f;
};

struct ChgModelData {
    std::string modelFileName;
};

// ============================================================================
// ChgFrameDefinition
// ============================================================================

struct ChgFrameDefinition {
    uint32_t frameType = CHG_FT_DUMMY;
    S_vector position = {0, 0, 0};
    S_vector scale = {1, 1, 1};
    S_quat rotation = {0, 0, 0, 1};
    std::string frameName;
    std::string parentName;

    // Type-specific data (populated based on frameType)
    ChgLightData lightData;
    ChgSoundData soundData;
    ChgModelData modelData;
};

// ============================================================================
// Actor/Script entries
// ============================================================================

struct ChgActorOnNewFrame {
    uint32_t actorType = 0;
    std::vector<uint8_t> rawData;
};

struct ChgActorOnExistingFrame {
    std::string frameName;
    uint32_t actorType = 0;
    std::vector<uint8_t> extraData;
};

struct ChgScriptData {
    std::vector<uint8_t> rawData;
};

// ============================================================================
// Unified entry
// ============================================================================

struct ChgEntry {
    enum Type {
        ENTRY_FRAME_DEFINITION,
        ENTRY_ACTOR_ON_NEW,
        ENTRY_ACTOR_ON_EXISTING,
        ENTRY_SCRIPT
    } type;

    ChgFrameDefinition frameDef;
    ChgActorOnNewFrame actorNew;
    ChgActorOnExistingFrame actorExisting;
    ChgScriptData scriptData;
};

// ============================================================================
// ChgFile container
// ============================================================================

struct ChgFile {
    std::vector<ChgEntry> entries;

    void Clear() { entries.clear(); }

    size_t CountFrameDefinitions() const {
        size_t count = 0;
        for (const auto& e : entries)
            if (e.type == ChgEntry::ENTRY_FRAME_DEFINITION) count++;
        return count;
    }

    size_t CountActorsOnNew() const {
        size_t count = 0;
        for (const auto& e : entries)
            if (e.type == ChgEntry::ENTRY_ACTOR_ON_NEW) count++;
        return count;
    }

    size_t CountActorsOnExisting() const {
        size_t count = 0;
        for (const auto& e : entries)
            if (e.type == ChgEntry::ENTRY_ACTOR_ON_EXISTING) count++;
        return count;
    }

    size_t CountScripts() const {
        size_t count = 0;
        for (const auto& e : entries)
            if (e.type == ChgEntry::ENTRY_SCRIPT) count++;
        return count;
    }
};

// ============================================================================
// Load / Save
// ============================================================================

bool LoadChgFile(const std::string& path, ChgFile& outFile);
bool SaveChgFile(const std::string& path, const ChgFile& file);
