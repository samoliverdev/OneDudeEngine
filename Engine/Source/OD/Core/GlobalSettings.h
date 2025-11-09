#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/SerializationFull.h"
#include "ImGui.h"
#include <unordered_map>
#include <string>
#include <functional>
#include <typeindex>
#include <memory>

namespace OD {

class GlobalSettings {
public:
    struct Section{
        std::string name;
        std::function<void()> drawer;
        std::function<void(ODOutputArchive&)> saveFunc;
        std::function<void(ODInputArchive&)> loadFunc;
    };

    static GlobalSettings& Get(){
        static GlobalSettings instance;
        return instance;
    }

    // Save/Load all settings
    void Save(const std::string& path);
    void Load(const std::string& path);

    // Render an ImGui panel with all settings
    void OnImGuiRender();

    // Register a settings class type
    template<typename T>
    void Register(const std::string& name){
        const std::type_index type = typeid(T);

        // Prevent duplicate registration
        if(settingsObjects.count(type))
            return;

        auto ptr = std::make_shared<T>();
        settingsObjects[type] = ptr;

        // Build default section for ImGui + Serialization
        Section section;
        section.name = name;
        section.drawer = [ptr]() {
            //if constexpr(requires(T t) { t.OnImGuiRender(); }){
                ptr->OnImGuiRender();
            //} else {
            //    ImGui::Text("No ImGuiRender() implemented for %s", typeid(T).name());
            //}
        };
        section.saveFunc = [ptr, name](ODOutputArchive& ar){ 
            //ar & *ptr; 
            //ArchiveDump(ar, *ptr);
            ArchiveDumpNamed(ar, name.c_str(), *ptr);
        };
        section.loadFunc = [ptr, name](ODInputArchive& ar){ 
            //ar & *ptr;
            ArchiveDumpNamed(ar, name.c_str(), *ptr); 
        };

        sections[name] = section;
        typeToName[type] = name;
    }

    // Get settings instance by type
    template<typename T>
    T& Get(){
        const std::type_index type = typeid(T);
        auto it = settingsObjects.find(type);
        if(it == settingsObjects.end()){
            throw std::runtime_error(std::string("GlobalSettings: Type not registered -> ") + typeid(T).name());
        }
        return *static_cast<T*>(it->second.get());
    }

    inline auto& GetSections(){ return sections; } 

private:
    GlobalSettings() = default;

    std::unordered_map<std::string, Section> sections;
    std::unordered_map<std::type_index, std::shared_ptr<void>> settingsObjects;
    std::unordered_map<std::type_index, std::string> typeToName;
};

} // namespace OD