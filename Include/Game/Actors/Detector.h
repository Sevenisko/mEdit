#pragma once

#include <Game/Actor.h>
#include <imgui.h>

struct DetectorData {
    uint8_t unk1;
    uint32_t enableSpatialDetector;
    uint8_t unk2;
    uint32_t unk3;
};

class Detector : public Actor {
  public:
    Detector() {
        m_Type = ACTOR_DETECTOR;
        ZeroMemory(&m_Data, sizeof(DetectorData));
    }

    void Duplicate(Actor* sourceActor) override {
        Actor::Duplicate(sourceActor);
        Detector* source = (Detector*)sourceActor;

        m_Data.unk1 = source->m_Data.unk1;
        m_Data.enableSpatialDetector = source->m_Data.enableSpatialDetector;
        m_Data.unk2 = source->m_Data.unk2;
        m_Data.unk3 = source->m_Data.unk3;
        m_Script.frame = GetFrame();
        m_Script.name = source->m_Script.name;
        m_Script.script = source->m_Script.script;
    }

    bool OnLoad(ChunkReader* chunk) override {
        m_Data.unk1 = chunk->Read<uint8_t>();
        m_Data.enableSpatialDetector = chunk->Read<uint32_t>();
        m_Data.unk2 = chunk->Read<uint8_t>();
        m_Data.unk3 = chunk->Read<uint32_t>();
        m_Script.frame = GetFrame();
        m_Script.name = GetFrame()->GetName();
        m_Script.script = chunk->ReadString();

        return true;
    }

    void OnSave(BinaryWriter* writer) override {
        writer->WriteUInt8(m_Data.unk1);
        writer->WriteUInt32(m_Data.enableSpatialDetector);
        writer->WriteUInt8(m_Data.unk2);
        writer->WriteUInt32(m_Data.unk3);
        writer->WriteString(m_Script.script);
    }

    void OnInspectorGUI() override {
        bool enableSpatial = m_Data.enableSpatialDetector != 0;
        ImGui::Checkbox("Enable spatial detector", &enableSpatial);
        m_Data.enableSpatialDetector = enableSpatial;

        if(ImGui::Button("Open script")) { SceneEditor::Get()->GetScriptEditor()->Open(&m_Script); }
    }

    GameScript* GetScript() override { return &m_Script;
    }

    size_t GetDataSize() override { return sizeof(DetectorData); }

    DetectorData* GetData() { return &m_Data; }

  private:
    DetectorData m_Data;
    GameScript m_Script;
};