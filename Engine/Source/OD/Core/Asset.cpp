#include "Asset.h"

namespace OD{

AssetManager globalAssetManager;

std::string& Asset::Path(){ 
    return path; 
}

bool Asset::LoadFromFile(const std::string& path){ 
    return false; 
}

std::vector<std::string> Asset::GetFileAssociations(){ 
    return std::vector<std::string>(); 
}

bool Asset::HasFileExtension(const std::string& fileExtension){
    for(auto& i: GetFileAssociations()){
        if(i == fileExtension) return true;
    }
    return false;
}

bool AssetTypesDB::HasAssetByExtension(std::string fileExtension){
    return assetFuncs.find(fileExtension) != assetFuncs.end();
}

AssetTypesDB& AssetTypesDB::Get(){
    static AssetTypesDB global;
    return global;
}

void AssetManager::UnloadAll(){
    data.clear();
}

AssetManager& AssetManager::Get(){
    return globalAssetManager;
}

}