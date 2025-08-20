#include "Heightmap.h"
#include "OD/Serialization/SerializationFull.h"
#include <fstream>

namespace OD{

bool Heightmap::LoadFromFile(const std::string& _path){
    path = _path;
    std::ifstream os(path, std::ios::binary);
    cereal::PortableBinaryInputArchive archive{os};
    archive(*this);
    return true;
}

bool Heightmap::SaveAs(const std::string& _path){
    path = _path;
    std::ofstream os(path, std::ios::binary);
    cereal::PortableBinaryOutputArchive archive{os};
    archive(*this);
    return true;
}

std::vector<std::string> Heightmap::GetFileAssociations(){
    return {".heightmap"};
}

}