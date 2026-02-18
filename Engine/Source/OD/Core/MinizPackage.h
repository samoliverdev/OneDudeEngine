#pragma once
#include "Package.h"
#include <miniz.h>

namespace OD{

class OD_API MinizPackage: public Package{
public:
    MinizPackage(const char* zipPath);
    ~MinizPackage();

    bool HasFile(const char* path) override;
    bool ReadFileData(const char* path, void*& outData, size_t& outSize) override;
    void FreeFileData(void*& data) override;

    size_t GetFileSize(const char* path) const override;
    bool ReadFile(const char* path, std::vector<uint8_t>& outData) override;
    std::vector<std::string> ListFiles(const char* directory) const override;

private:
    mz_zip_archive m_zip{};
};

}