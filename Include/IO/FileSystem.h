#pragma once

#include <rw_data.h>
#include <string>
#include <vector>
#include <map>

class FileSystem {
public:
    struct File {
        std::string name;
        size_t size;
    };

    struct Directory {
        std::string name;
        std::vector<File> files;
        std::vector<Directory> directories;
    };

	static void Init();

    static Directory* GetRootDir();

private:
    static bool LoadFileIndex();
    static void BuildFileIndex();

	static std::map<std::string, size_t> g_Files;
};
