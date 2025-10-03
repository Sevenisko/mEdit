#pragma once

#include <Game/Actor.h>
#include <imgui.h>
#include <imgui_stdlib.h>

struct PlayerData {
    uint8_t unk;
    uint32_t characterType;
    CharacterVoice voice;
    float strength;
    float energy;
    float leftHandEnergy;
    float rightHandEnergy;
    float leftLegEnergy;
    float rightLegEnergy;
    float reactions;
    float speed;
    float aggresivity;
    float intelligence;
    float shooting;
    float sight;
    float hearing;
    float driving;
    float mass;
    float morale;
};

class Player : public Actor {
  public:
    Player() {
        m_Type = ACTOR_PLAYER;
        ZeroMemory(&m_Data, sizeof(PlayerData));
        m_Data.characterType = 1;
        m_Data.voice = VOICE_TOMMY;
        m_Data.strength = 0.7f;
        m_Data.energy = 200.0f;
        m_Data.leftHandEnergy = 40.0f;
        m_Data.rightHandEnergy = 40.0f;
        m_Data.leftLegEnergy = 40.0f;
        m_Data.rightLegEnergy = 40.0f;
        m_Data.reactions = 0.7f;
        m_Data.speed = 1.0f;
        m_Data.aggresivity = 0.6f;
        m_Data.intelligence = 0.8f;
        m_Data.shooting = 1.0f;
        m_Data.sight = 1.0f;
        m_Data.hearing = 1.0f;
        m_Data.driving = 0.8f;
        m_Data.mass = 80.0f;
        m_Data.morale = 0.5f;
    }

    void Duplicate(Actor* sourceActor) override { 
        Actor::Duplicate(sourceActor); 
        Player* source = (Player*)sourceActor;

        m_Data.unk = source->m_Data.unk;
        m_Data.characterType = source->m_Data.characterType;
        m_Data.voice = source->m_Data.voice;
        m_Data.strength = source->m_Data.strength;
        m_Data.energy = source->m_Data.energy;
        m_Data.leftHandEnergy = source->m_Data.leftHandEnergy;
        m_Data.rightHandEnergy = source->m_Data.rightHandEnergy;
        m_Data.leftLegEnergy = source->m_Data.leftLegEnergy;
        m_Data.rightLegEnergy = source->m_Data.rightLegEnergy;
        m_Data.reactions = source->m_Data.reactions;
        m_Data.speed = source->m_Data.speed;
        m_Data.aggresivity = source->m_Data.aggresivity;
        m_Data.intelligence = source->m_Data.intelligence;
        m_Data.shooting = source->m_Data.shooting;
        m_Data.sight = source->m_Data.sight;
        m_Data.hearing = source->m_Data.hearing;
        m_Data.driving = source->m_Data.driving;
        m_Data.mass = source->m_Data.mass;
        m_Data.morale = source->m_Data.morale;
    }

    bool OnLoad(ChunkReader* chunk) override {
        m_Data.unk = chunk->Read<uint8_t>();
        m_Data.characterType = chunk->Read<uint32_t>();
        m_Data.voice = chunk->Read<CharacterVoice>();
        m_Data.strength = chunk->Read<float>();
        m_Data.energy = chunk->Read<float>();
        m_Data.leftHandEnergy = chunk->Read<float>();
        m_Data.rightHandEnergy = chunk->Read<float>();
        m_Data.leftLegEnergy = chunk->Read<float>();
        m_Data.rightLegEnergy = chunk->Read<float>();
        m_Data.reactions = chunk->Read<float>();
        m_Data.speed = chunk->Read<float>();
        m_Data.aggresivity = chunk->Read<float>();
        m_Data.intelligence = chunk->Read<float>();
        m_Data.shooting = chunk->Read<float>();
        m_Data.sight = chunk->Read<float>();
        m_Data.hearing = chunk->Read<float>();
        m_Data.driving = chunk->Read<float>();
        m_Data.mass = chunk->Read<float>();
        m_Data.morale = chunk->Read<float>();

        return true;
    }
    void OnSave(BinaryWriter* writer) override {
        writer->WriteUInt8(m_Data.unk);
        writer->WriteUInt32(m_Data.characterType);
        writer->WriteUInt32(m_Data.voice);
        writer->WriteSingle(m_Data.strength);
        writer->WriteSingle(m_Data.energy);
        writer->WriteSingle(m_Data.leftHandEnergy);
        writer->WriteSingle(m_Data.rightHandEnergy);
        writer->WriteSingle(m_Data.leftLegEnergy);
        writer->WriteSingle(m_Data.rightLegEnergy);
        writer->WriteSingle(m_Data.reactions);
        writer->WriteSingle(m_Data.speed);
        writer->WriteSingle(m_Data.aggresivity);
        writer->WriteSingle(m_Data.intelligence);
        writer->WriteSingle(m_Data.shooting);
        writer->WriteSingle(m_Data.sight);
        writer->WriteSingle(m_Data.hearing);
        writer->WriteSingle(m_Data.driving);
        writer->WriteSingle(m_Data.mass);
        writer->WriteSingle(m_Data.morale);
    }

    void OnInspectorGUI() override {
        ImGui::InputInt("Character type", (int*)&m_Data.characterType);
        ImGui::InputInt("Voice", (int*)&m_Data.voice);
        ImGui::DragFloat("Strength", &m_Data.strength);
        ImGui::DragFloat("Energy (Health)", &m_Data.energy);
        ImGui::DragFloat("Left hand energy", &m_Data.leftHandEnergy);
        ImGui::DragFloat("Right hand energy", &m_Data.rightHandEnergy);
        ImGui::DragFloat("Left leg energy", &m_Data.leftLegEnergy);
        ImGui::DragFloat("Right leg energy", &m_Data.rightLegEnergy);
        ImGui::DragFloat("Reactions", &m_Data.reactions);
        ImGui::DragFloat("Speed", &m_Data.speed);
        ImGui::DragFloat("Aggresivity", &m_Data.aggresivity);
        ImGui::DragFloat("Intelligence", &m_Data.intelligence);
        ImGui::DragFloat("Shooting", &m_Data.shooting);
        ImGui::DragFloat("Sight", &m_Data.sight);
        ImGui::DragFloat("Hearing", &m_Data.hearing);
        ImGui::DragFloat("Driving", &m_Data.driving);
        ImGui::DragFloat("Mass", &m_Data.mass);
        ImGui::DragFloat("Morale", &m_Data.morale);
    }

    size_t GetDataSize() override { return sizeof(PlayerData); }

    PlayerData* GetData() { return &m_Data; }

  private:
    PlayerData m_Data;
};