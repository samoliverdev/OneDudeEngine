#include "OD/pch.h"
#include "Resource.h"
#include <execution>
#include <filesystem>

namespace OD{

std::string& Resource::Path(){ 
    return path; 
}

bool Resource::PathIsValid(){
    if(path.empty()) return false;
    if(path[0] == '#') return false;
    return true;
}

void Resource::OnGui(){

}

void Resource::Reload(){ 
    LoadFromFile(path); 
}

bool Resource::Save(const std::string& outPath, SaveType type){
    return false; 
}

bool Resource::LoadFromFile(const std::string& path){
    Assert(false && "Not Implemented"); 
    return false; 
}

bool Resource::LoadFromPackage(const std::string& path, Package& package){
    Assert(false && "Not Implemented");
    return false;
}

std::vector<std::string> Resource::GetFileAssociations(){ 
    return std::vector<std::string>(); 
}

bool Resource::HasFileExtension(const std::string& fileExtension){
    for(auto& i: GetFileAssociations()){
        if(i == fileExtension) return true;
    }
    return false;
}

/*Ref<Package> Asset::GetPackage(){
    return package;
}*/

}