#pragma once
#include "Scripts.h"
#include "SceneManager.h"

namespace OD{
    
template <class Archive>
void ScriptComponent::serialize(Archive& ar){
    /*if(SceneManager::Get().scriptsSerializerByType.count())

    if constexpr (Archive::is_loading()){

    } else {
        std::vector<std::type_index> typeIndexs;
        for(auto& i: instances){
            typeIndexs.push_back(std::type_index(typeid(*i.second.instance));
            //LogInfo("%s", typeid(*i.second.instance).name());
        }

        ArchiveDumpNVP(ar, typeIndexs);
        for(auto& i: typeIndexs){
            SceneManager::Get().scriptsSerializerByType[i].compSave()
        }
    }*/

    if constexpr(std::is_same_v<Archive, ODOutputArchive>){
        std::vector<std::string> typeIds;
        std::vector<Script*> _instances;

        for(auto& i: instances){
            for(auto& j: SceneManager::Get().scriptsSerializer){
                if(j.second.getType() == std::type_index(typeid(*i.second.instance))){
                    typeIds.push_back(j.first);
                    _instances.push_back(i.second.instance);
                    break;
                }
            }
        }

        ArchiveDumpNVP(ar, typeIds);
        for(int i = 0; i < typeIds.size(); i++){
            SceneManager::Get().scriptsSerializer[typeIds[i]].scriptSave(ar, _instances[i]);
        }
    }

    if constexpr(std::is_same_v<Archive, ODInputArchive>){
        std::vector<std::string> typeIds;
        ArchiveDumpNVP(ar, typeIds);

        for(int i = 0; i < typeIds.size(); i++){
            Script* instance = SceneManager::Get().scriptsSerializer[typeIds[i]].addScript(*this);
            SceneManager::Get().scriptsSerializer[typeIds[i]].scriptLoad(ar, instance);
        }
    }
}

}