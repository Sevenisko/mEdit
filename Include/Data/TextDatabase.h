#pragma once

#include <string>
#include <vector>

class TextDatabase {
  public:
    struct TextEntry {
        uint32_t uTextId;
        std::string szText;
    };

    bool Load(const std::string& szFilePath);

    bool Save();

    void Clear() { m_vEntries.clear(); }

    size_t NumEntries() { return m_vEntries.size(); }

    TextEntry* GetEntry(int index);
    std::string GetText(uint32_t id);

  private:
    std::string m_szFileName;
    std::vector<TextEntry> m_vEntries;
};