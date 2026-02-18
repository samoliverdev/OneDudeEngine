#include "PhysFSPackage.h"
#include "OD/Base.h"

namespace OD{

class OD_API PhysFSPackage: public Package{
public:
    bool HasFile(const char* path) override;

    bool ReadFileData(const char* path, void*& outData, size_t& outSize) override;
    void FreeFileData(void*& data) override;

    size_t GetFileSize(const char* path) const override;
    bool ReadFile(const char* path, std::vector<uint8_t>& outData) override;
    std::vector<std::string> ListFiles(const char* directory) const override;
};

PhysFSPackage package;

void PhysFS::Init(){
    PHYSFS_init(nullptr);
    Assert(PHYSFS_isInit());
}

bool PhysFS::Mount(const char* path, bool highPriority){
    int r = PHYSFS_mount(path, "/", highPriority ? 0 : 1);
    if(!r){
        const char* err = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        LogFatal("PhysFS mount error: {}", err);
        Assert(false);
    }

    return true;
}

bool PhysFS::Unmount(const char* path){
    int r = PHYSFS_unmount(path);
    return r != 0;
}

void PhysFS::Shutdown(){
    PHYSFS_deinit();
}

Package* PhysFS::GetPackage(){
    return &package;
}

bool PhysFSPackage::HasFile(const char* path){
    return PHYSFS_exists(path) != 0;
}

size_t PhysFSPackage::GetFileSize(const char* path) const{
    PHYSFS_File* file = PHYSFS_openRead(path);
    if(!file) return 0;

    PHYSFS_sint64 size = PHYSFS_fileLength(file);
    PHYSFS_close(file);

    return (size_t)size;
}

bool PhysFSPackage::ReadFileData(const char* path, void*& outData, size_t& outSize){
    PHYSFS_File* file = PHYSFS_openRead(path);
    if(!file) return false;

    PHYSFS_sint64 size = PHYSFS_fileLength(file);
    if(size <= 0){
        PHYSFS_close(file);
        return false;
    }

    outData = malloc((size_t)size);
    outSize = (size_t)size;

    PHYSFS_readBytes(file, outData, size);
    PHYSFS_close(file);
    return true;
}

void PhysFSPackage::FreeFileData(void*& data){
    if(data){
        free(data);
        data = nullptr;
    }
}

bool PhysFSPackage::ReadFile(const char* path, std::vector<uint8_t>& outData){
    PHYSFS_File* file = PHYSFS_openRead(path);
    if(!file) return false;

    PHYSFS_sint64 size = PHYSFS_fileLength(file);
    if(size <= 0){
        PHYSFS_close(file);
        return false;
    }

    outData.resize((size_t)size);
    PHYSFS_readBytes(file, outData.data(), size);
    PHYSFS_close(file);

    return true;
}

std::vector<std::string> PhysFSPackage::ListFiles(const char* directory) const{
    std::vector<std::string> result;

    char** rc = PHYSFS_enumerateFiles(directory);
    char** i = rc;

    while(*i != nullptr){
        result.emplace_back(directory + std::string("/") + *i);
        i++;
    }

    PHYSFS_freeList(rc);

    return result;
}


}