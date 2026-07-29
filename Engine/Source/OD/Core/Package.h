#pragma once
#include "OD/Defines.h"
#include "OD/Base.h"
#include <vector>
#include <string>

namespace OD{

class OD_API Package{
public:
    virtual ~Package() = default;

    virtual bool HasFile(const char* path) = 0;
    virtual bool ReadFileData(const char* path, void*& outData, size_t& outSize) = 0;
    virtual void FreeFileData(void*& data) = 0;

    virtual size_t GetFileSize(const char* path) const { return 0; }
    virtual bool ReadFile(const char* path, std::vector<uint8_t>& outData){ return false; };
    virtual std::vector<std::string> ListFiles(const char* directory) const{ return std::vector<std::string>(); };
    virtual std::vector<std::string> ListFilesRecursive(const char* directory) const{ Assert(false && "Not Implemented"); return std::vector<std::string>(); };
};

//TODO: Implement later
/*class Package(System|Manager) {
public:
    void Mount(Package* p);
    bool ReadFile(const char* path, void*& outData, size_t& outSize);
private:
    std::vector<Package*> packages;
};*/


}