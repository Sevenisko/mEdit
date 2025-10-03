#pragma once

#include <Game/Actor.h>
#include <imgui.h>
#include <imgui_stdlib.h>

struct RailwayData {
    uint8_t unk1;
    uint32_t wagonCount;
    float wagonDistance;
    float unk2;
    float maxSpeed;
    float unk3;
    float unk4;
};

class Railway : public Actor {
  public:
    Railway() {
        m_Type = ACTOR_RAILWAY;
        ZeroMemory(&m_Data, sizeof(RailwayData));
    }

    void Duplicate(Actor* sourceActor) override { 
        Actor::Duplicate(sourceActor); 
        Railway* source = (Railway*)sourceActor;

        m_Data.unk1 = source->m_Data.unk1;
        m_Data.wagonCount = source->m_Data.wagonCount;
        m_Data.wagonDistance = source->m_Data.wagonDistance;
        m_Data.unk2 = source->m_Data.unk2;
        m_Data.maxSpeed = source->m_Data.maxSpeed;
        m_Data.unk3 = source->m_Data.unk3;
        m_Data.unk4 = source->m_Data.unk4;
    }

    bool OnLoad(ChunkReader* chunk) override {
        m_Data.unk1 = chunk->Read<uint8_t>();
        m_Data.wagonCount = chunk->Read<uint32_t>();
        m_Data.wagonDistance = chunk->Read<float>();
        m_Data.unk2 = chunk->Read<float>();
        m_Data.maxSpeed = chunk->Read<float>();
        m_Data.unk3 = chunk->Read<float>();
        m_Data.unk4 = chunk->Read<float>();

        return true;
    }

    void OnSave(BinaryWriter* writer) override {
        writer->WriteUInt8(m_Data.unk1);
        writer->WriteUInt32(m_Data.wagonCount);
        writer->WriteSingle(m_Data.wagonDistance);
        writer->WriteSingle(m_Data.unk2);
        writer->WriteSingle(m_Data.maxSpeed);
        writer->WriteSingle(m_Data.unk3);
        writer->WriteSingle(m_Data.unk4);
    }

    void OnInspectorGUI() override {
        ImGui::InputInt("Wagon count", (int*)&m_Data.wagonCount);
        ImGui::DragFloat("Wagon distance", &m_Data.wagonDistance);
        ImGui::DragFloat("Max speed", &m_Data.maxSpeed);
    }

    size_t GetDataSize() override { return sizeof(RailwayData); }

    RailwayData* GetData() { return &m_Data; }

  private:
    RailwayData m_Data;
};