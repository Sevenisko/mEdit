#include <Data/TextDatabase.h>
#include <IO/BinaryReader.hpp>
#include <IO/BinaryWriter.hpp>
#include <Windows.h>
#include <EditorApplication.h>

inline wchar_t* CodePageToUnicode(int codePage, const char* src) {
    if(!src) return 0;
    int srcLen = strlen(src);
    if(!srcLen) {
        wchar_t* w = new wchar_t[1];
        w[0] = 0;
        return w;
    }

    int requiredSize = MultiByteToWideChar(codePage, 0, src, srcLen, 0, 0);

    if(!requiredSize) { return 0; }

    wchar_t* w = new wchar_t[requiredSize + 1];
    w[requiredSize] = 0;

    int retval = MultiByteToWideChar(codePage, 0, src, srcLen, w, requiredSize);
    if(!retval) {
        delete[] w;
        return 0;
    }

    return w;
}

inline char* UnicodeToCodePage(int codePage, const wchar_t* src) {
    if(!src) return 0;
    int srcLen = wcslen(src);
    if(!srcLen) {
        char* x = new char[1];
        x[0] = '\0';
        return x;
    }

    int requiredSize = WideCharToMultiByte(codePage, 0, src, srcLen, 0, 0, 0, 0);

    if(!requiredSize) { return 0; }

    char* x = new char[requiredSize + 1];
    x[requiredSize] = 0;

    int retval = WideCharToMultiByte(codePage, 0, src, srcLen, x, requiredSize, 0, 0);
    if(!retval) {
        delete[] x;
        return 0;
    }

    return x;
}

static std::string ConvertWindows1250ToUTF8(const std::string& str) {
    auto wText = CodePageToUnicode(1250, str.c_str());

    return std::string(UnicodeToCodePage(65001, wText));
}

static std::string ConvertUTF8ToWindows1250(const std::string& str) {
    auto wText = CodePageToUnicode(65001, str.c_str());

    return std::string(UnicodeToCodePage(1250, wText));
}

static std::string ConvertWindows1251ToUTF8(const std::string& str) {
    auto wText = CodePageToUnicode(1251, str.c_str());

    return std::string(UnicodeToCodePage(65001, wText));
}

static std::string ConvertUTF8ToWindows1251(const std::string& str) {
    auto wText = CodePageToUnicode(65001, str.c_str());

    return std::string(UnicodeToCodePage(1251, wText));
}

static std::string ConvertWindows1252ToUTF8(const std::string& str) {
    auto wText = CodePageToUnicode(1252, str.c_str());

    return std::string(UnicodeToCodePage(65001, wText));
}

static std::string ConvertUTF8ToWindows1252(const std::string& str) {
    auto wText = CodePageToUnicode(65001, str.c_str());

    return std::string(UnicodeToCodePage(1252, wText));
}

struct FileTextEntry {
    uint32_t uTextId;
    uint32_t uTextPos;
};

bool TextDatabase::Load(const std::string& szFilePath) {
    m_szFileName = szFilePath;
    m_vEntries.clear();

    BinaryReader reader(szFilePath);

    if(reader.IsOpen()) {
        uint32_t uNumEntries, uUnk;
        uNumEntries = reader.ReadUInt32();
        uUnk = reader.ReadUInt32();

        m_vEntries.resize(uNumEntries);

        for(uint32_t u = 0; u < uNumEntries; u++) {
            TextEntry& sEntry = m_vEntries[u];
            sEntry.uTextId = reader.ReadUInt32();
            uint32_t uTextPos = reader.ReadUInt32();
            auto uCurPos = reader.GetCurPos();
            reader.Seek(uTextPos, SEEK_SET);
            std::string str = reader.ReadNullTerminatedString();
            if(szFilePath.find("cz") != szFilePath.npos) {
                sEntry.szText = ConvertWindows1250ToUTF8(str);
            } else if(szFilePath.find("ru") != szFilePath.npos) {
                sEntry.szText = ConvertWindows1251ToUTF8(str);
            } else if(szFilePath.find("de") != szFilePath.npos) {
                sEntry.szText = ConvertWindows1252ToUTF8(str);
            } else {
                sEntry.szText = str;
            }

            reader.Seek(uCurPos, SEEK_SET);
        }

        return true;
    } else {
        LogError("Failed to open %s for reading: %s", m_szFileName.c_str(), std::strerror(errno));
        return false;
    }
}

bool TextDatabase::Save() {
    std::vector<FileTextEntry> vFileEntries;
    vFileEntries.resize(m_vEntries.size());

    int i = 0;

    for(TextEntry& entry: m_vEntries) {
        FileTextEntry& fileEntry = vFileEntries[i];
        fileEntry.uTextId = entry.uTextId;
        i++;
    }

    uint32_t uTextOffset = (2 * sizeof(uint32_t)) + (vFileEntries.size() * sizeof(FileTextEntry));

    i = 0;
    uint32_t uTotalTextSize = 0;

    for(FileTextEntry& entry: vFileEntries) {
        TextEntry& textEntry = m_vEntries[i];
        entry.uTextPos = uTextOffset + uTotalTextSize;
        uTotalTextSize += textEntry.szText.length() + 1;
        i++;
    }

    uint32_t uNumEntries = vFileEntries.size();
    uint32_t uUnk = 0;

    BinaryWriter writer(m_szFileName);

    if(!writer.IsOpen()) {
        LogError("Failed to open %s for writing: %s", m_szFileName.c_str(), std::strerror(errno));
        return false;
    } else {
        writer.WriteUInt32(uNumEntries);
        writer.WriteUInt32(uUnk);

        for(FileTextEntry& entry: vFileEntries) {
            writer.WriteUInt32(entry.uTextId);
            writer.WriteUInt32(entry.uTextPos);
        }

        for(TextEntry& entry: m_vEntries) {
            std::string szText;

            if(m_szFileName.find("cz") != m_szFileName.npos) {
                szText = ConvertUTF8ToWindows1250(entry.szText);
            } else if(m_szFileName.find("ru") != m_szFileName.npos) {
                szText = ConvertUTF8ToWindows1251(entry.szText);
            } else if(m_szFileName.find("de") != m_szFileName.npos) {
                szText = ConvertUTF8ToWindows1252(entry.szText);
            } else {
                szText = entry.szText;
            }
            writer.WriteNullTerminatedString(szText);
        }
    }

    return true;
}

TextDatabase::TextEntry* TextDatabase::GetEntry(int index) {
    if(index < 0 || (size_t)index >= m_vEntries.size()) { return nullptr; }

    return &m_vEntries[index];
}

std::string TextDatabase::GetText(uint32_t id) {
    for(TextEntry& entry: m_vEntries) {
        if(entry.uTextId == id) return entry.szText;
    }

    return "";
}