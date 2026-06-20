#include <IO/ChgFile.h>
#include <IO/BinaryReader.hpp>
#include <IO/BinaryWriter.hpp>
#include <IO/ChunkReader.hpp>
#include <IO/ChunkWriter.hpp>

extern void editorLog(const char* fmt, ...);
extern void editorLogWarning(const char* fmt, ...);
extern void editorLogError(const char* fmt, ...);

// ============================================================================
// LoadChgFile
// ============================================================================

bool LoadChgFile(const std::string& path, ChgFile& outFile) {
    outFile.Clear();

    BinaryReader reader(path);
    if (!reader.IsOpen()) {
        editorLogError("ChgFile: Failed to open %s", path.c_str());
        return false;
    }

    ChunkReader chunk(&reader);

    // Enter root chunk
    auto rootType = ++chunk;
    if (rootType != CHG_ROOT) {
        editorLogError("ChgFile: Invalid root chunk type %d (expected %d)", rootType, CHG_ROOT);
        return false;
    }

    while (chunk) {
        auto type = ++chunk;

        switch (type) {
        case CHG_FRAME_DEFINITION: {
            ChgEntry entry;
            entry.type = ChgEntry::ENTRY_FRAME_DEFINITION;
            auto& fd = entry.frameDef;

            fd.frameType = reader.ReadUInt32();
            fd.position = reader.ReadVec3();
            fd.scale = reader.ReadVec3();
            fd.rotation = reader.ReadQuatWXYZ();
            fd.frameName = reader.ReadString();
            fd.parentName = reader.ReadString();

            switch (fd.frameType) {
            case CHG_FT_LIGHT: {
                fd.lightData.lightType = reader.ReadUInt32();
                fd.lightData.color = reader.ReadVec3();
                fd.lightData.range = reader.ReadSingle();
                fd.lightData.isOn = reader.ReadUInt8();
                fd.lightData.coneInner = reader.ReadSingle();
                fd.lightData.coneOuter = reader.ReadSingle();
                fd.lightData.falloffNear = reader.ReadSingle();
                fd.lightData.falloffFar = reader.ReadSingle();
                fd.lightData.lightMode = reader.ReadUInt32();

                uint32_t sectorCount = reader.ReadUInt32();
                for (uint32_t i = 0; i < sectorCount; i++) {
                    uint32_t len = reader.ReadUInt32();
                    std::string sectorName = reader.ReadFixedString(len + 1); // includes null terminator
                    // Trim trailing null
                    if (!sectorName.empty() && sectorName.back() == '\0')
                        sectorName.pop_back();
                    fd.lightData.sectorLinks.push_back(sectorName);
                }
                break;
            }
            case CHG_FT_SOUND: {
                fd.soundData.soundFileName = reader.ReadString();
                fd.soundData.param0 = reader.ReadSingle();
                fd.soundData.isOn = reader.ReadUInt8();
                fd.soundData.soundParam1 = reader.ReadSingle();
                fd.soundData.soundParam2 = reader.ReadSingle();
                fd.soundData.soundParam3 = reader.ReadSingle();
                fd.soundData.soundParam4 = reader.ReadSingle();
                fd.soundData.rangeNear = reader.ReadSingle();
                fd.soundData.rangeFar = reader.ReadSingle();
                fd.soundData.coneAngle = reader.ReadSingle();
                fd.soundData.ambient = reader.ReadSingle();
                fd.soundData.loop = reader.ReadUInt8();
                fd.soundData.volume = reader.ReadSingle();
                break;
            }
            case CHG_FT_MODEL: {
                fd.modelData.modelFileName = reader.ReadString();
                break;
            }
            default:
                // Other frame types have no extra data
                break;
            }

            outFile.entries.push_back(std::move(entry));
            break;
        }

        case CHG_ACTOR_ON_NEW: {
            ChgEntry entry;
            entry.type = ChgEntry::ENTRY_ACTOR_ON_NEW;
            entry.actorNew.actorType = reader.ReadUInt32();

            size_t remaining = chunk.GetRemainingBytes();
            if (remaining > 0) {
                entry.actorNew.rawData.resize(remaining);
                reader.Read(entry.actorNew.rawData.data(), remaining);
            }

            outFile.entries.push_back(std::move(entry));
            break;
        }

        case CHG_ACTOR_ON_EXISTING: {
            ChgEntry entry;
            entry.type = ChgEntry::ENTRY_ACTOR_ON_EXISTING;
            entry.actorExisting.frameName = reader.ReadString();
            entry.actorExisting.actorType = reader.ReadUInt32();

            size_t remaining = chunk.GetRemainingBytes();
            if (remaining > 0) {
                entry.actorExisting.extraData.resize(remaining);
                reader.Read(entry.actorExisting.extraData.data(), remaining);
            }

            outFile.entries.push_back(std::move(entry));
            break;
        }

        case CHG_SCRIPT: {
            ChgEntry entry;
            entry.type = ChgEntry::ENTRY_SCRIPT;

            size_t remaining = chunk.GetRemainingBytes();
            if (remaining > 0) {
                entry.scriptData.rawData.resize(remaining);
                reader.Read(entry.scriptData.rawData.data(), remaining);
            }

            outFile.entries.push_back(std::move(entry));
            break;
        }

        default:
            editorLogWarning("ChgFile: Unknown chunk type %d, skipping", type);
            break;
        }

        --chunk;
    }

    --chunk; // close root

    editorLog("ChgFile: Loaded %d entries (%d frames, %d new actors, %d existing actors, %d scripts)",
        (int)outFile.entries.size(),
        (int)outFile.CountFrameDefinitions(),
        (int)outFile.CountActorsOnNew(),
        (int)outFile.CountActorsOnExisting(),
        (int)outFile.CountScripts());

    return true;
}

// ============================================================================
// SaveChgFile
// ============================================================================

bool SaveChgFile(const std::string& path, const ChgFile& file) {
    BinaryWriter writer(path);
    if (!writer.IsOpen()) {
        editorLogError("ChgFile: Failed to create %s", path.c_str());
        return false;
    }

    ChunkWriter chunk(&writer);

    // Root chunk
    chunk += CHG_ROOT;

    for (const auto& entry : file.entries) {
        switch (entry.type) {
        case ChgEntry::ENTRY_FRAME_DEFINITION: {
            chunk += CHG_FRAME_DEFINITION;
            const auto& fd = entry.frameDef;

            writer.WriteUInt32(fd.frameType);
            writer.WriteVec3(fd.position);
            writer.WriteVec3(fd.scale);
            writer.WriteQuatWXYZ(fd.rotation);
            writer.WriteString(fd.frameName);
            writer.WriteString(fd.parentName);

            switch (fd.frameType) {
            case CHG_FT_LIGHT: {
                writer.WriteUInt32(fd.lightData.lightType);
                writer.WriteVec3(fd.lightData.color);
                writer.WriteSingle(fd.lightData.range);
                writer.WriteUInt8(fd.lightData.isOn);
                writer.WriteSingle(fd.lightData.coneInner);
                writer.WriteSingle(fd.lightData.coneOuter);
                writer.WriteSingle(fd.lightData.falloffNear);
                writer.WriteSingle(fd.lightData.falloffFar);
                writer.WriteUInt32(fd.lightData.lightMode);

                writer.WriteUInt32((uint32_t)fd.lightData.sectorLinks.size());
                for (const auto& sectorName : fd.lightData.sectorLinks) {
                    writer.WriteUInt32((uint32_t)sectorName.length());
                    // Write string + null terminator
                    for (char c : sectorName) {
                        writer.WriteUInt8((uint8_t)c);
                    }
                    writer.WriteUInt8(0); // null terminator
                }
                break;
            }
            case CHG_FT_SOUND: {
                writer.WriteString(fd.soundData.soundFileName);
                writer.WriteSingle(fd.soundData.param0);
                writer.WriteUInt8(fd.soundData.isOn);
                writer.WriteSingle(fd.soundData.soundParam1);
                writer.WriteSingle(fd.soundData.soundParam2);
                writer.WriteSingle(fd.soundData.soundParam3);
                writer.WriteSingle(fd.soundData.soundParam4);
                writer.WriteSingle(fd.soundData.rangeNear);
                writer.WriteSingle(fd.soundData.rangeFar);
                writer.WriteSingle(fd.soundData.coneAngle);
                writer.WriteSingle(fd.soundData.ambient);
                writer.WriteUInt8(fd.soundData.loop);
                writer.WriteSingle(fd.soundData.volume);
                break;
            }
            case CHG_FT_MODEL: {
                writer.WriteString(fd.modelData.modelFileName);
                break;
            }
            default:
                break;
            }

            --chunk;
            break;
        }

        case ChgEntry::ENTRY_ACTOR_ON_NEW: {
            chunk += CHG_ACTOR_ON_NEW;
            writer.WriteUInt32(entry.actorNew.actorType);
            if (!entry.actorNew.rawData.empty()) {
                writer.Write((void*)entry.actorNew.rawData.data(), entry.actorNew.rawData.size());
            }
            --chunk;
            break;
        }

        case ChgEntry::ENTRY_ACTOR_ON_EXISTING: {
            chunk += CHG_ACTOR_ON_EXISTING;
            writer.WriteString(entry.actorExisting.frameName);
            writer.WriteUInt32(entry.actorExisting.actorType);
            if (!entry.actorExisting.extraData.empty()) {
                writer.Write((void*)entry.actorExisting.extraData.data(), entry.actorExisting.extraData.size());
            }
            --chunk;
            break;
        }

        case ChgEntry::ENTRY_SCRIPT: {
            chunk += CHG_SCRIPT;
            if (!entry.scriptData.rawData.empty()) {
                writer.Write((void*)entry.scriptData.rawData.data(), entry.scriptData.rawData.size());
            }
            --chunk;
            break;
        }
        }
    }

    --chunk; // close root

    editorLog("ChgFile: Saved %d entries to %s", (int)file.entries.size(), path.c_str());
    return true;
}
