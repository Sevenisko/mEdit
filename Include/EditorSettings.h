#pragma once

#include <json.hpp>
#include <fstream>

struct EditorSettings {
    bool IsLoaded() const { return m_IsLoaded; }

    void Load(const std::string& fileName) {
        std::ifstream file(fileName);

        nlohmann::json json = nlohmann::json::parse(file, nullptr, false);

        if(!json.is_discarded()) {
            video.currentAdapter = json["Video"]["CurrentAdapter"];
            video.width = json["Video"]["Width"];
            video.height = json["Video"]["Height"];
            video.refreshRate = json["Video"]["RefreshRate"];
            video.fullscreen = json["Video"]["Fullscreen"];
            video.vsync = json["Video"]["VSync"];

            m_IsLoaded = true;
        }
    }

    void Save(const std::string& fileName) {
        nlohmann::json json;

        json["Video"]["CurrentAdapter"] = video.currentAdapter;
        json["Video"]["Width"] = video.width;
        json["Video"]["Height"] = video.height;
        json["Video"]["RefreshRate"] = video.refreshRate;
        json["Video"]["Fullscreen"] = video.fullscreen;
        json["Video"]["VSync"] = video.vsync;

        std::ofstream file(fileName);
        file << std::setw(4) << json << std::endl;
        file.close();

        m_IsLoaded = true;
    }

    struct VideoSettings {
        int currentAdapter = 0;
        int width = 1280;
        int height = 720;
        int refreshRate = 60;
        bool fullscreen = false;
        bool vsync = false;
    } video;

  private:
    bool m_IsLoaded = false;
};