#include <Data/ItemsDatabase.h>
#include <EditorApplication.h>
#include <fstream>

bool ItemsDatabase::Load(const std::string& szFilePath) {
    m_szFileName = szFilePath;

    std::ifstream file(szFilePath, std::ios::binary);

    if(!file.is_open()) {
        LogError("Failed to open %s for reading: %s", szFilePath.c_str(), std::strerror(errno));
        return false;
    } else {
        file.seekg(0, file.end);
        auto size = static_cast<size_t>(file.tellg());
        file.seekg(0);
        m_vItems.resize(size / sizeof(Item));
        file.read((char*)m_vItems.data(), size);
        file.close();
        return true;
    }
}

bool ItemsDatabase::Save() {
    std::ofstream file(m_szFileName, std::ios::binary);

    if(!file.is_open()) {
        LogError("Failed to open %s for writing: %s", m_szFileName.c_str(), std::strerror(errno));
        return false;
    } else {
        file.write((char*)m_vItems.data(), m_vItems.size() * sizeof(Item));
        file.close();
        return true;
    }
}