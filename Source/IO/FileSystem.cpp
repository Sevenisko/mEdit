#include <IO/FileSystem.h>
#include <Utils.h>
#include <map>
#include <fstream>
#include <algorithm>
#include <json.hpp>
#include <IO/BinaryWriter.hpp>

struct DTAKeys {
    uint32_t uKey1, uKey2;
};

std::map<std::string, DTAKeys> g_DTAFiles{
    {"A0.dta", {0x7F3D9B74, 0xEC48FE17}}, // Sounds
    {"A1.dta", {0xE7375F59, 0x0900210E}}, // Missions
    {"A2.dta", {0x1417D340, 0xB6399E19}}, // Models
    {"A3.dta", {0xA94B8D3C, 0x771F3888}}, // Anims I
    {"A4.dta", {0xA94B8D3C, 0x771F3888}}, // Anims II
    {"A5.dta", {0x4F4BB0C6, 0xEA340420}}, // Diff data
    {"A6.dta", {0x728E2DB9, 0x5055DA68}}, // Textures
    {"A7.dta", {0xF4F03A72, 0xE266FE62}}, // Records (cutscenes)
    {"A8.dta", {0x6A63FA71, 0xEC45D8CE}}, // Patch data
    {"A9.dta", {0x959D1117, 0x5B763446}}, // System data
    {"AA.dta", {0xD4AD90C6, 0x67DA216E}}, // Tables
    {"AB.dta", {0x7F3D9B74, 0xEC48FE17}}, // Music
    {"AC.dta", {0xA94B8D3C, 0x771F3888}} // Anims III
};

/*
 * This is the initial header, it contains 4 fields:
 *
 *  4 bytes     - Number of files in this archive
 *  4 bytes     - Beginning offset of the content table
 *  4 bytes     - Size of the content table
 *  4 bytes     - [Unknown]
 */
typedef struct t_dtaheader {
    DWORD numOfFiles;
    DWORD contentOffset;
    DWORD contentSize;
    DWORD extra;
} DTA_HEADER;

/*
 * This is the contents header, it contains 4 fields as well:
 *
 *  4 bytes     - [Unknown]
 *  4 bytes     - Beginning offset of the file data
 *  4 bytes     - [Unknown]
 *  16 bytes    - Filename hint
 */
typedef struct t_dtacontentheader {
    DWORD extra1;
    DWORD fileOffset;
    DWORD extra2;
    char filename[16];
} DTA_CONTENT_HEADER;

/*
 *
 *
 */
typedef struct t_dtafileheader {
    DWORD extra1;
    DWORD extra2;
    DWORD extra3;
    DWORD extra4;
    DWORD fileSize;
    DWORD extra5;
    unsigned char filenameLength;
    char extra6[7];
    /*char            *filename; Processed seperately */
} DTA_FILE_HEADER;

struct DTAInfo {
    int dtaHandle;
    int numOfFiles;
    uint32_t key1, key2;
} g_DTAInfo;

#define DTA_OPEN_FAILED     -1
#define MAGIC_KEY1          0x034985762
#define MAGIC_KEY2          0x039475694
#define TRUE_DTA_IDENTIFIER (('I') + ('S' << 8) + ('D' << 16) + ('0' << 24))

void __stdcall Decrypt(void* buf, size_t size, uint32_t key1, uint32_t key2) {
    uint32_t uKeys[2] = {key1 ^ 0x39475694, key2 ^ 0x34985762};
    uint8_t* uKeyBytes = (uint8_t*)uKeys;

    for(uint32_t i = 0; i < size; ++i) {
        uint8_t uDataByte = ((uint8_t*)buf)[i];
        uint8_t uKeyByte = uKeyBytes[i % sizeof(uKeys)];
        ((uint8_t*)buf)[i] = (uint8_t)(~((~uDataByte) ^ uKeyByte));
    }
}

static std::string ProcessFile() {
    char filename[256 + 1] = {0};
    DTA_FILE_HEADER fileHeader = {0};

    dtaRead(g_DTAInfo.dtaHandle, (char*)&fileHeader, sizeof(DTA_FILE_HEADER));
    Decrypt((void*)&fileHeader, sizeof(DTA_FILE_HEADER), g_DTAInfo.key1, g_DTAInfo.key2);

    dtaRead(g_DTAInfo.dtaHandle, filename, fileHeader.filenameLength);
    Decrypt((void*)filename, fileHeader.filenameLength, g_DTAInfo.key1, g_DTAInfo.key2);
    filename[fileHeader.filenameLength] = '\0';

    return filename;
}

bool ProcessDTAFiles(std::map<std::string, size_t>& files) {
    DTA_CONTENT_HEADER* contentHeaders;

    contentHeaders = (DTA_CONTENT_HEADER*)malloc(sizeof(DTA_CONTENT_HEADER) * g_DTAInfo.numOfFiles);

    if(!contentHeaders) return false;

    dtaRead(g_DTAInfo.dtaHandle, (char*)contentHeaders, sizeof(DTA_CONTENT_HEADER) * g_DTAInfo.numOfFiles);
    Decrypt((void*)contentHeaders, sizeof(DTA_CONTENT_HEADER) * g_DTAInfo.numOfFiles, g_DTAInfo.key1, g_DTAInfo.key2);

    while(g_DTAInfo.numOfFiles--) {
        int pos = contentHeaders[g_DTAInfo.numOfFiles].fileOffset;
        dtaSeek(g_DTAInfo.dtaHandle, pos, SEEK_SET);

        std::string name = ProcessFile();

        size_t size = 0;
        int descriptor = dtaOpen(name.c_str(), 0);
        if(descriptor != -1) {
            size = dtaSeek(descriptor, 0, SEEK_END);
            dtaClose(descriptor);
        }

        if(files.find(name) == files.end()) {
            //debugPrintf("Added file to VFS: %s (%u bytes)", name.c_str(), size);
            files.insert({name, size});
        }
    }

    free(contentHeaders);
    return true;
}

bool ProcessDTAHeader() {
    int identifier;
    DTA_HEADER header = {0};

    dtaRead(g_DTAInfo.dtaHandle, (char*)&identifier, sizeof(int));

    if(identifier != TRUE_DTA_IDENTIFIER) { return false; }

    dtaRead(g_DTAInfo.dtaHandle, (char*)&header, sizeof(DTA_HEADER));
    Decrypt((void*)&header, sizeof(DTA_HEADER), g_DTAInfo.key1, g_DTAInfo.key2);

    g_DTAInfo.numOfFiles = header.numOfFiles;

    dtaSeek(g_DTAInfo.dtaHandle, header.contentOffset, SEEK_SET);

    return true;
}

bool ProcessDTAFile(const std::string& szDtaFile, uint32_t uKey1, uint32_t uKey2) {
    DTA_CONTENT_HEADER c = {0};
    DTA_HEADER f = {0};
    DTA_FILE_HEADER h = {0};

    g_DTAInfo.key1 = uKey1;
    g_DTAInfo.key2 = uKey2;

    g_DTAInfo.dtaHandle = dtaOpen(szDtaFile.c_str(), 0);

    if(g_DTAInfo.dtaHandle == DTA_OPEN_FAILED) { return false; }

    return true;
}

std::map<std::string, size_t> FetchFilesFromDTA() {
    std::map<std::string, size_t> files;

    for(auto entry: g_DTAFiles) {
        if(!ProcessDTAFile(entry.first, entry.second.uKey1, entry.second.uKey2)) { continue; }

        if(!ProcessDTAHeader()) { continue; }

        if(!ProcessDTAFiles(files)) { continue; }
    }

    return files;
}

FileSystem::Directory g_RootDir;
std::map<std::string, size_t> FileSystem::g_Files{};

void FileSystem::Init() {
    dtaSetDtaFirstForce();

    for(auto entry: g_DTAFiles) {
        C_rw_data* dtaFile = dtaCreate(entry.first.c_str());
        if(dtaFile && dtaFile->UnlockPack(entry.second.uKey1, entry.second.uKey2)) {
            //debugPrintf("Loaded DTA file: %s", entry.first.c_str());
        } else {
            debugPrintf("Failed to load DTA file: %s", entry.first.c_str());
        }
    }

    if(!LoadFileIndex()) BuildFileIndex();
}

FileSystem::Directory* FileSystem::GetRootDir() { return &g_RootDir; }

static std::string NormalizeDirName(const std::string& name) {
    if(name.empty()) return name;
    std::string result = name;
    result[0] = std::toupper(result[0]);
    for(size_t i = 1; i < result.size(); ++i) {
        result[i] = std::tolower(result[i]);
    }
    return result;
}

static std::string LowercaseString(const std::string& name) {
    if(name.empty()) return name;
    std::string result = name;
    for(size_t i = 0; i < result.size(); ++i) {
        result[i] = std::tolower(result[i]);
    }
    return result;
}

FileSystem::Directory& FindOrCreateDirectory(FileSystem::Directory& parent, const std::string& name) {
    std::string normalizedName = NormalizeDirName(name);
    for(auto& dir: parent.directories) {
        if(parent.name == g_RootDir.name) {
            if(dir.name == normalizedName) { return dir; }
        } else {
            if(LowercaseString(dir.name) == LowercaseString(name)) { return dir; }
        }
    }
    parent.directories.emplace_back();
    parent.directories.back().name = parent.name == g_RootDir.name ? normalizedName : name;
    return parent.directories.back();
}

static std::string ReadString(std::ifstream& in) {
    size_t length;
    in.read(reinterpret_cast<char*>(&length), sizeof(length));
    std::string str(length, '\0');
    in.read(str.data(), length);
    return str;
}

// Helper method to write a string to binary file
static void WriteString(std::ofstream& out, const std::string& str) {
    size_t length = str.size();
    out.write(reinterpret_cast<const char*>(&length), sizeof(length));
    out.write(str.data(), length);
}

void ReadDirectory(std::ifstream& in, FileSystem::Directory& dir) {
    dir.name = ReadString(in);

    size_t numFiles;
    in.read(reinterpret_cast<char*>(&numFiles), sizeof(numFiles));
    dir.files.resize(numFiles);
    for(size_t i = 0; i < numFiles; ++i) {
        dir.files[i].name = ReadString(in);
        in.read(reinterpret_cast<char*>(&dir.files[i].size), sizeof(size_t));
    }

    size_t numDirectories;
    in.read(reinterpret_cast<char*>(&numDirectories), sizeof(numDirectories));
    dir.directories.resize(numDirectories);
    for(size_t i = 0; i < numDirectories; ++i) {
        ReadDirectory(in, dir.directories[i]);
    }
}

void WriteDirectory(std::ofstream& out, const FileSystem::Directory& dir) {
    WriteString(out, dir.name);

    size_t numFiles = dir.files.size();
    out.write(reinterpret_cast<const char*>(&numFiles), sizeof(numFiles));
    for(const auto& file: dir.files) {
        WriteString(out, file.name);
        out.write(reinterpret_cast<const char*>(&file.size), sizeof(size_t));
    }

    size_t numDirectories = dir.directories.size();
    out.write(reinterpret_cast<const char*>(&numDirectories), sizeof(numDirectories));
    for(const auto& subDir: dir.directories) {
        WriteDirectory(out, subDir);
    }
}

bool FileSystem::LoadFileIndex() {
    // Read JSON file
    std::ifstream file("mEdit\\fileindex.dat", std::ios::binary);
    if(!file.is_open()) { return false; }

    ReadDirectory(file, g_RootDir);

    file.close();

    return true;
}

void FileSystem::BuildFileIndex() {
    g_Files = FetchFilesFromDTA();

    g_RootDir = Directory{"Game", {}, {}};

    for(const auto& entry: g_Files) {
        std::stringstream ss(entry.first);
        std::string segment;
        Directory* currentDir = &g_RootDir;
        std::vector<std::string> segments;

        // Split path into segments
        while(std::getline(ss, segment, '\\')) {
            if(!segment.empty()) { segments.push_back(segment); }
        }

        // Process each segment
        for(size_t i = 0; i < segments.size(); ++i) {
            const auto& seg = segments[i];
            if(i == segments.size() - 1) {
                // Last segment is a file
                bool fileExists = false;
                for(const auto& file: currentDir->files) {
                    if(file.name == seg) {
                        fileExists = true;
                        break;
                    }
                }
                if(!fileExists) { currentDir->files.push_back({seg, entry.second}); }
            } else {
                // Intermediate segment is a directory
                currentDir = &FindOrCreateDirectory(*currentDir, seg);
            }
        }
    }

    std::ofstream file("mEdit\\fileindex.dat", std::ios::binary);

    if(file.is_open()) {
        WriteDirectory(file, g_RootDir);

        file.close();
    }
}