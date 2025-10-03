#pragma once

#include <Game/Actor.h>
#include <imgui.h>
#include <imgui_stdlib.h>

struct DoorData {
    uint32_t unk1;
    uint8_t unk2;
    bool openInBothDirections;
    uint8_t forcedOpenDirection;
    float openAngle;
    bool opened;
    bool locked;
    float openSpeed;
    float closeSpeed;
    std::string openSound;
    std::string closeSound;
    std::string lockedSound;
    uint8_t unk3;
};

class Door : public Actor {
  public:
    Door() {
        m_Type = ACTOR_DOOR;
        ZeroMemory(&m_Data, sizeof(DoorData));
    }

    void Duplicate(Actor* sourceActor) override {
        Actor::Duplicate(sourceActor);
        Door* source = (Door*)sourceActor;

        m_Data.unk1 = source->m_Data.unk1;
        m_Data.unk2 = source->m_Data.unk2;
        m_Data.openInBothDirections = source->m_Data.openInBothDirections;
        m_Data.forcedOpenDirection = source->m_Data.forcedOpenDirection;
        m_Data.openAngle = source->m_Data.openAngle;
        m_Data.opened = source->m_Data.opened;
        m_Data.locked = source->m_Data.locked;
        m_Data.openSpeed = source->m_Data.openAngle;
        m_Data.closeSpeed = source->m_Data.closeSpeed;
        m_Data.openSound = source->m_Data.openSound;
        m_Data.closeSound = source->m_Data.closeSound;
        m_Data.lockedSound = source->m_Data.lockedSound;
        m_Data.unk3 = source->m_Data.unk3;
    }

    bool OnLoad(ChunkReader* chunk) override {
        m_Data.unk1 = chunk->Read<uint32_t>();
        m_Data.unk2 = chunk->Read<uint8_t>();
        m_Data.openInBothDirections = chunk->Read<bool>();
        m_Data.forcedOpenDirection = chunk->Read<uint8_t>();
        m_Data.openAngle = chunk->Read<float>();
        m_Data.opened = chunk->Read<bool>();
        m_Data.locked = chunk->Read<bool>();
        m_Data.openSpeed = chunk->Read<float>();
        m_Data.closeSpeed = chunk->Read<float>();
        m_Data.openSound = chunk->ReadFixedString(16);
        m_Data.closeSound = chunk->ReadFixedString(16);
        m_Data.lockedSound = chunk->ReadFixedString(16);
        m_Data.unk3 = chunk->Read<uint8_t>();

        return true;
    }

    void OnSave(BinaryWriter* writer) override {
        writer->WriteUInt32(m_Data.unk1);
        writer->WriteUInt8(m_Data.unk2);
        writer->WriteBoolean(m_Data.openInBothDirections);
        writer->WriteUInt8(m_Data.forcedOpenDirection);
        writer->WriteSingle(m_Data.openAngle);
        writer->WriteBoolean(m_Data.opened);
        writer->WriteBoolean(m_Data.locked);
        writer->WriteSingle(m_Data.openSpeed);
        writer->WriteSingle(m_Data.closeSpeed);
        writer->WriteFixedString(m_Data.openSound, 16);
        writer->WriteFixedString(m_Data.closeSound, 16);
        writer->WriteFixedString(m_Data.lockedSound, 16);
        writer->WriteUInt8(m_Data.unk3);
    }

    void OnInspectorGUI() override { 
        ImGui::Checkbox("Open in both directions", &m_Data.openInBothDirections);
        ImGui::DragFloat("Open angle", &m_Data.openAngle);
        ImGui::Checkbox("Is opened", &m_Data.opened);
        ImGui::Checkbox("Is locked", &m_Data.locked);
        ImGui::DragFloat("Open speed", &m_Data.openSpeed);
        ImGui::DragFloat("Close speed", &m_Data.closeSpeed);
        ImGui::InputText("Open sound", &m_Data.openSound);
        ImGui::InputText("Close sound", &m_Data.closeSound);
        ImGui::InputText("Locked sound", &m_Data.lockedSound);
    }

    size_t GetDataSize() override { return sizeof(DoorData); }

    DoorData* GetData() { return &m_Data; }

  private:
    DoorData m_Data;
};