#pragma once

#include <Game/Actor.h>

struct TrafficCar {
    std::string vehicle;
    float densityRatio;
    uint8_t color;
    uint8_t unk1;
    uint8_t unk2;
    uint8_t unk3;
    bool police;
    bool gangster1;
    bool gangster2;
    uint8_t unk4;
};

struct TrafficManagerData {
    uint32_t unk;
    float despawnRadius;
    float innerSpawnRadius;
    float outerSpawnRadius;
    uint32_t maxCarsCount;
    std::vector<TrafficCar> cars;
};

class TrafficManager : public Actor {
  public:
    TrafficManager() {
        m_Type = ACTOR_TRAFFIC;
        ZeroMemory(&m_Data, sizeof(TrafficManagerData));
    }

    void Duplicate(Actor* sourceActor) override {
        Actor::Duplicate(sourceActor);
        TrafficManager* source = (TrafficManager*)sourceActor;

        m_Data.unk = source->m_Data.unk;
        m_Data.despawnRadius = source->m_Data.despawnRadius;
        m_Data.innerSpawnRadius = source->m_Data.innerSpawnRadius;
        m_Data.outerSpawnRadius = source->m_Data.outerSpawnRadius;
        m_Data.maxCarsCount = source->m_Data.maxCarsCount;
        m_Data.cars = source->m_Data.cars;
    }

    bool OnLoad(ChunkReader* chunk) override {
        m_Data.unk = chunk->Read<uint32_t>();
        m_Data.despawnRadius = chunk->Read<float>();
        m_Data.innerSpawnRadius = chunk->Read<float>();
        m_Data.outerSpawnRadius = chunk->Read<float>();
        m_Data.maxCarsCount = chunk->Read<uint32_t>();
        uint32_t carsCount = chunk->Read<uint32_t>();
        m_Data.cars.resize(carsCount);
        for(uint32_t i = 0; i < carsCount; i++) {
            TrafficCar& car = m_Data.cars[i];
            car.vehicle = chunk->ReadFixedString(20);
            car.densityRatio = chunk->Read<float>();
            car.color = chunk->Read<uint8_t>();
            car.unk1 = chunk->Read<uint8_t>();
            car.unk2 = chunk->Read<uint8_t>();
            car.unk3 = chunk->Read<uint8_t>();
            car.police = chunk->Read<bool>();
            car.gangster1 = chunk->Read<bool>();
            car.gangster2 = chunk->Read<bool>();
            car.unk4 = chunk->Read<uint8_t>();
        }

        return true;
    }

    void OnSave(BinaryWriter* writer) override {
        writer->WriteUInt32(m_Data.unk);
        writer->WriteSingle(m_Data.despawnRadius);
        writer->WriteSingle(m_Data.innerSpawnRadius);
        writer->WriteSingle(m_Data.outerSpawnRadius);
        writer->WriteUInt32(m_Data.maxCarsCount);
        writer->WriteUInt32(m_Data.cars.size());
        for(TrafficCar& car: m_Data.cars) {
            writer->WriteFixedString(car.vehicle, 20);
            writer->WriteSingle(car.densityRatio);
            writer->WriteUInt8(car.color);
            writer->WriteUInt8(car.unk1);
            writer->WriteUInt8(car.unk2);
            writer->WriteUInt8(car.unk3);
            writer->WriteBoolean(car.police);
            writer->WriteBoolean(car.gangster1);
            writer->WriteBoolean(car.gangster2);
            writer->WriteUInt8(car.unk4);
        }
    }

    void OnInspectorGUI() override {
        ImGui::DragFloat("Despawn radius", &m_Data.despawnRadius);
        ImGui::DragFloat("Inner spawn radius", &m_Data.innerSpawnRadius);
        ImGui::DragFloat("Outer spawn radius", &m_Data.outerSpawnRadius);
        ImGui::InputInt("Max spawned cars", (int*)&m_Data.maxCarsCount);

        std::vector<int> toRemove;

        if(ImGui::BeginChild("#cars")) {
            int i = 0;
            if(m_Data.cars.empty()) {
                ImGui::Text("Empty...");
            } else {
                for(auto& car: m_Data.cars) {
                    ImGui::PushID(i); // Unique ID for widgets
                    ImGui::Text("%d", i);
                    ImGui::SameLine();
                    if(ImGui::Button("Remove")) { toRemove.push_back(i); }
                    ImGui::Separator();
                    ImGui::InputText("Vehicle", &car.vehicle);
                    ImGui::SliderFloat("Density ratio", &car.densityRatio, 0, 100);
                    ImGui::InputInt("Color", (int*)&car.color, 1, 1);
                    ImGui::Checkbox("Is police patrol", &car.police);
                    ImGui::Checkbox("Is gangster car", &car.gangster1);
                    ImGui::Checkbox("Has gangster passenger", &car.gangster1);
                    ImGui::Separator();
                    ImGui::NewLine();
                    ImGui::PopID();
                    i++;
                }
            }

            if(ImGui::Button("Add")) { m_Data.cars.emplace_back(); }

            ImGui::EndChild();
        }

        for(int index: toRemove) {
            m_Data.cars.erase(m_Data.cars.begin() + index);
        }
    }

    size_t GetDataSize() override { return sizeof(TrafficManagerData); }

    TrafficManagerData* GetData() { return &m_Data; }

  private:
    TrafficManagerData m_Data;
};