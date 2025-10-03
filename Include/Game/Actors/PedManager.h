#pragma once

#include <Game/Actor.h>
#include <imgui.h>

struct PedManagerData {
    uint32_t unk1;
    uint8_t unk2;
    float spawnRadius;
    float despawnRadius;
    float innerSpawnRadius;
    float outerSpawnRadius;
    float innerSpawnRadius2;
    uint32_t maxPedsCount;
    std::vector<std::string> pedModels;
    std::vector<uint32_t> pedDensities;
};

class PedManager : public Actor {
  public:
    PedManager() {
        m_Type = ACTOR_PEDESTRIANS;
        ZeroMemory(&m_Data, sizeof(PedManagerData));
    }

    void Duplicate(Actor* sourceActor) override {
        Actor::Duplicate(sourceActor);
        PedManager* source = (PedManager*)sourceActor;

        m_Data.unk1 = source->m_Data.unk1;
        m_Data.unk2 = source->m_Data.unk2;
        m_Data.spawnRadius = source->m_Data.spawnRadius;
        m_Data.despawnRadius = source->m_Data.despawnRadius;
        m_Data.innerSpawnRadius = source->m_Data.innerSpawnRadius;
        m_Data.outerSpawnRadius = source->m_Data.outerSpawnRadius;
        m_Data.innerSpawnRadius2 = source->m_Data.innerSpawnRadius2;
        m_Data.maxPedsCount = source->m_Data.maxPedsCount;
        m_Data.pedModels = source->m_Data.pedModels;
        m_Data.pedDensities = source->m_Data.pedDensities;
    }

    bool OnLoad(ChunkReader* chunk) override {
        m_Data.unk1 = chunk->Read<uint32_t>();
        m_Data.unk2 = chunk->Read<uint8_t>();
        m_Data.spawnRadius = chunk->Read<float>();
        m_Data.despawnRadius = chunk->Read<float>();
        m_Data.innerSpawnRadius = chunk->Read<float>();
        m_Data.outerSpawnRadius = chunk->Read<float>();
        m_Data.innerSpawnRadius2 = chunk->Read<float>();
        m_Data.maxPedsCount = chunk->Read<uint32_t>();
        uint32_t pedsCount = chunk->Read<uint32_t>();
        m_Data.pedModels.resize(pedsCount);
        m_Data.pedDensities.resize(pedsCount);

        for(uint32_t i = 0; i < pedsCount; i++) {
            m_Data.pedModels[i] = chunk->ReadFixedString(17);
        }

        for(uint32_t i = 0; i < pedsCount; i++) {
            m_Data.pedDensities[i] = chunk->Read<uint32_t>();
        }

        return true;
    }

    void OnSave(BinaryWriter* writer) override {
        writer->WriteUInt32(m_Data.unk1);
        writer->WriteUInt8(m_Data.unk2);
        writer->WriteSingle(m_Data.spawnRadius);
        writer->WriteSingle(m_Data.despawnRadius);
        writer->WriteSingle(m_Data.innerSpawnRadius);
        writer->WriteSingle(m_Data.outerSpawnRadius);
        writer->WriteSingle(m_Data.innerSpawnRadius2);
        writer->WriteUInt32(m_Data.maxPedsCount);
        writer->WriteUInt32(m_Data.pedModels.size());

        for(auto model: m_Data.pedModels) {
            writer->WriteFixedString(model, 17);
        }

        for(auto density: m_Data.pedDensities) {
            writer->WriteUInt32(density);
        }
    }

    void OnInspectorGUI() override {
        ImGui::DragFloat("Spawn radius", &m_Data.spawnRadius);
        ImGui::DragFloat("Despawn radius", &m_Data.despawnRadius);
        ImGui::DragFloat("Inner spawn radius", &m_Data.innerSpawnRadius);
        ImGui::DragFloat("Outer spawn radius", &m_Data.outerSpawnRadius);
        ImGui::DragFloat("Inner spawn radius (2)", &m_Data.innerSpawnRadius2);
        ImGui::InputInt("Max spawned PEDs", (int*)&m_Data.maxPedsCount);

        std::vector<int> toRemove;

        ImGui::Text("Ped Models");
        ImGui::Separator();
        if(ImGui::BeginChild("#PEDs")) {
            int i = 0;
            if(m_Data.pedModels.empty()) {
                ImGui::Text("Empty...");
            } else {
                for(auto pedModel: m_Data.pedModels) {
                    ImGui::PushID(i); // Unique ID for widgets
                    ImGui::Text("%d", i);
                    ImGui::SameLine();
                    if(ImGui::Button("Remove")) { toRemove.push_back(i); }
                    ImGui::Separator();
                    ImGui::InputText("Model", &m_Data.pedModels[i]);
                    ImGui::SliderInt("Density", (int*)&m_Data.pedDensities[i], 0, 100);
                    ImGui::Separator();
                    ImGui::NewLine();
                    ImGui::PopID();
                    i++;
                }
            }

            if(ImGui::Button("Add")) {
                m_Data.pedModels.emplace_back();
                m_Data.pedDensities.emplace_back();
            }

            ImGui::EndChild();
        }
        ImGui::Separator();

        for(int index: toRemove) {
            m_Data.pedModels.erase(m_Data.pedModels.begin() + index);
            m_Data.pedDensities.erase(m_Data.pedDensities.begin() + index);
        }
    }

    size_t GetDataSize() override { return sizeof(PedManagerData); }

    PedManagerData* GetData() { return &m_Data; }

  private:
    PedManagerData m_Data;
};