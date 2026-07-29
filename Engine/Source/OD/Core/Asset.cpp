#include "OD/pch.h"
#include "Asset.h"
#include <execution>
#include <filesystem>

namespace OD{

std::string& Asset::Path(){ 
    return path; 
}

bool Asset::PathIsValid(){
    if(path.empty()) return false;
    if(path[0] == '#') return false;
    return true;
}

void Asset::OnGui(){

}

void Asset::Reload(){ 
    LoadFromFile(path); 
}

bool Asset::Save(const std::string& outPath, SaveType type){
    return false; 
}

bool Asset::LoadFromFile(const std::string& path){
    Assert(false && "Not Implemented"); 
    return false; 
}

bool Asset::LoadFromPackage(const std::string& path, Package& package){
    Assert(false && "Not Implemented");
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

/*Ref<Package> Asset::GetPackage(){
    return package;
}*/

}