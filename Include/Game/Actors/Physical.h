#pragma once

#include <Game/Actor.h>
#include <imgui.h>

struct PhysicalData {
    uint8_t unk1;
    uint8_t collide;
    float unk2;
    float unk3;
    float mass;
    float friction;
    float unk4;
    PhysicsType sound;
    uint8_t unk5;
    Material hitMaterial;
};

class Physical : public Actor {
  public:
    Physical() {
        m_Type = ACTOR_PHYSICAL;
        ZeroMemory(&m_Data, sizeof(PhysicalData));
    }

    void Duplicate(Actor* sourceActor) override {
        Actor::Duplicate(sourceActor);
        Physical* source = (Physical*)sourceActor;

        m_Data.unk1 = source->m_Data.unk1;
        m_Data.collide = source->m_Data.collide;
        m_Data.unk2 = source->m_Data.unk2;
        m_Data.unk3 = source->m_Data.unk3;
        m_Data.mass = source->m_Data.mass;
        m_Data.friction = source->m_Data.friction;
        m_Data.unk4 = source->m_Data.unk4;
        m_Data.sound = source->m_Data.sound;
        m_Data.unk5 = source->m_Data.unk5;
        m_Data.hitMaterial = source->m_Data.hitMaterial;
    }

    bool OnLoad(ChunkReader* chunk) override {
        m_Data.unk1 = chunk->Read<uint8_t>();
        m_Data.collide = chunk->Read<uint8_t>();
        m_Data.unk2 = chunk->Read<float>();
        m_Data.unk3 = chunk->Read<float>();
        m_Data.mass = chunk->Read<float>();
        m_Data.friction = chunk->Read<float>();
        m_Data.unk4 = chunk->Read<float>();
        m_Data.sound = chunk->Read<PhysicsType>();
        m_Data.unk5 = chunk->Read<uint8_t>();
        m_Data.hitMaterial = chunk->Read<Material>();

        return true;
    }

    void OnSave(BinaryWriter* writer) override {
        writer->WriteUInt8(m_Data.unk1);
        writer->WriteUInt8(m_Data.collide);
        writer->WriteSingle(m_Data.unk2);
        writer->WriteSingle(m_Data.unk3);
        writer->WriteSingle(m_Data.mass);
        writer->WriteSingle(m_Data.friction);
        writer->WriteSingle(m_Data.unk4);
        writer->WriteUInt32(m_Data.sound);
        writer->WriteUInt8(m_Data.unk5);
        writer->WriteUInt32(m_Data.hitMaterial);
    }

    void OnInspectorGUI() override {
        ImGui::Checkbox("Collide", (bool*)&m_Data.collide);
        ImGui::InputInt("Sound ID", (int*)&m_Data.sound);
        ImGui::InputInt("Material type", (int*)&m_Data.hitMaterial);
        ImGui::DragFloat("Mass", &m_Data.mass);
        ImGui::DragFloat("Friction", &m_Data.friction);
    }

    size_t GetDataSize() override { return sizeof(PhysicalData); }

    PhysicalData* GetData() { return &m_Data; }

  private:
    PhysicalData m_Data;
};