#pragma once
#include "OD/Defines.h"

namespace OD{

class OD_API Package{
public:
    virtual bool HasFile(const char* path) = 0;
    virtual bool ReadFileData(const char* path, void*& outData, size_t& outSize) = 0;
    virtual void FreeFileData(void*& data) = 0;
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