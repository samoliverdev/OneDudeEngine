#include "OD/pch.h"
#include "Heightmap.h"
#include "OD/Serialization/SerializationFull.h"

namespace OD{

bool Heightmap::LoadFromFile(const std::string& _path){
    path = _path;
    std::ifstream os(path, std::ios::binary);
    cereal::PortableBinaryInputArchive archive{os};
    archive(*this);
    return true;
}

bool Heightmap::Save(const std::string& outPath, SaveType type){
    if(type == SaveType::SettingOnly) return false;
    
    //path = _path;
    
    std::ofstream os(outPath, std::ios::binary);
    if(os.is_open() == false) return false;

    cereal::PortableBinaryOutputArchive archive{os};
    archive(*this);
    return true;
}

std::vector<std::string> Heightmap::GetFileAssociations(){
    return {".heightmap"};
}

}