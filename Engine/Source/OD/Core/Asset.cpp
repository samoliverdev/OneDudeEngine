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

std::unordered_map<std::string, Ref<Asset>>& AssetManager::GetDB(Type id){
    return data[id];
}

void AssetManager::UnloadAll(){
    //for(auto& [type, db]: data){
        /*for(auto& [path, asset]: db){
            LogWarning("Unload Asset: %s %d", asset->Path().c_str(), asset.use_count());
            Assert(asset.use_count() == 1);
            asset.reset();  // Explicitly release shared_ptr
        }*/
    //    db.clear();  // Clear inner map
    //}
    //data.clear();  // Then clear the outer map
    
    data.clear(); //Fixme: Crach in Debug Mode and using Engine.dll
}

AssetManager& AssetManager::Get(){
    return globalAssetManager;
}

}