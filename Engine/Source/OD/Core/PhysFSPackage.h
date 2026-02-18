#pragma once
#include "Package.h"
#include <physfs.h>

namespace OD{

class OD_API PhysFSPackage: public Package{
public:
    static void Init();
    static void Shutdown();

    static bool Mount(const char* path, bool highPriority = false);
    static bool Unmount(const char* path);

    bool HasFile(const char* path) override;

    bool ReadFileData(const char* path, void*& outData, size_t& outSize) override;
    void FreeFileData(void*& data) override;

    size_t GetFileSize(const char* path) const override;
    bool ReadFile(const char* path, std::vector<uint8_t>& outData) override;
    std::vector<std::string> ListFiles(const char* directory) const override;
};

}