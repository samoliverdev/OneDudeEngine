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
    std::vector<std::string> ListFilesRecursive(const char* directory) const override;
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

    /*while(*i != nullptr){
        result.emplace_back(directory + std::string("/") + *i);
        i++;
    }*/

    const char* separator = "";
    if(directory && *directory){
        size_t len = std::strlen(directory);
        char last = directory[len - 1];
        if(last != '/' && last != '\\'){
            separator = "/";
        }
    }
    while(*i != nullptr){
        result.emplace_back(std::string(directory) + separator + *i);
        i++;
    }

    PHYSFS_freeList(rc);

    return result;
}

std::vector<std::string> PhysFSPackage::ListFilesRecursive(const char* directory) const{
    std::vector<std::string> result;

    // Precompute separator
    const char* separator = "";
    if(directory && *directory){
        size_t len = std::strlen(directory);
        char last = directory[len - 1];
        if(last != '/' && last != '\\'){
            separator = "/";
        }
    }

    char** rc = PHYSFS_enumerateFiles(directory);
    char** i = rc;

    while(*i != nullptr){
        std::string fullPath = std::string(directory) + separator + *i;

        PHYSFS_Stat stat;
        if(PHYSFS_stat(fullPath.c_str(), &stat) != 0){
            /*if(stat.filetype == PHYSFS_FILETYPE_DIRECTORY){
                auto subFiles = ListFilesRecursive(fullPath.c_str());// Recurse into subdirectory
                result.insert(result.end(), subFiles.begin(), subFiles.end());
            } else {
                result.emplace_back(fullPath);// It's a file
            }*/

            if(stat.filetype == PHYSFS_FILETYPE_DIRECTORY){
                auto subFiles = ListFilesRecursive(fullPath.c_str());
                result.insert(result.end(), subFiles.begin(), subFiles.end());
            }
            else if(stat.filetype == PHYSFS_FILETYPE_REGULAR){
                result.emplace_back(fullPath);
            }
        }

        i++;
    }

    PHYSFS_freeList(rc);
    //LogInfo("Count: {}", result.size());
    return result;
}


}