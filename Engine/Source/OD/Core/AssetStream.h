#pragma once
#include <fstream>
#include <memory>
#include "Asset.h"

namespace OD{

class AssetStream{
public:
    AssetStream(const std::string& path, const std::vector<Package*>& packages){
        Open(path, packages);
    }

    ~AssetStream(){
        if(package && data) package->FreeFileData(data);
    }

    bool IsOpen() const { return stream != nullptr; }

    std::istream& GetStream() { return *stream; }

private:
    void Open(const std::string& path, const std::vector<Package*>& packages){
        //Try packages first
        for(auto* pkg : packages){
            if(pkg->HasFile(path.c_str())){
                size_t size = 0;
                if(pkg->ReadFileData(path.c_str(), data, size)){
                    package = pkg;
                    memoryStream = std::make_unique<MemoryInputStream>((char*)data, size);

                    stream = memoryStream.get();
                    return;
                }
            }
        }

        // Fallback to disk
        fileStream = std::make_unique<std::ifstream>(path, std::ios::binary);

        if(fileStream->is_open()) stream = fileStream.get();
    }

private:
    Package* package = nullptr;
    void* data = nullptr;

    std::unique_ptr<std::ifstream> fileStream;
    std::unique_ptr<MemoryInputStream> memoryStream;

    std::istream* stream = nullptr;
};

}