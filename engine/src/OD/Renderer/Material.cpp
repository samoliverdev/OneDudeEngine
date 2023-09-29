#include "Material.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include "OD/Serialization/Serialization.h"
#include "OD/Core/AssetManager.h"
#include "OD/Core/ImGui.h"

namespace OD{

void Material::SetFloat(const char* name, float value){
    MaterialMap& map = maps[name];

    //if(map.value == value) return;

    map.type = MaterialMap::Type::Float;
    map.value = value;
    map.isDirt = true;
    _isDirt = true;
}

void Material::SetVector2(const char* name, Vector2 value){
    MaterialMap& map = maps[name];

    //if(map.vector == Vector4(value.x, value.y, 0, 1)) return;

    map.type = MaterialMap::Type::Vector2;
    map.vector = Vector4(value.x, value.y, 0, 1);
    map.isDirt = true;
    _isDirt = true;
}

void Material::SetVector3(const char* name, Vector3 value){
    MaterialMap& map = maps[name];

    //if(map.vector == Vector4(value.x, value.y, value.z, 1)) return;

    map.type = MaterialMap::Type::Vector3;
    map.vector = Vector4(value.x, value.y, value.z, 1);
    map.isDirt = true;
    _isDirt = true;
}

void Material::SetVector4(const char* name, Vector4 value){
    MaterialMap& map = maps[name];

    //if(map.vector == value) return;

    map.type = MaterialMap::Type::Vector4;
    map.vector = value;
    map.isDirt = true;
    _isDirt = true;
}

void Material::SetTexture(const char* name, Ref<Texture2D> tex){
    MaterialMap& map = maps[name];

    //if(map.texture == tex) return;

    map.type = MaterialMap::Type::Texture;
    map.texture = tex;
    map.isDirt = true;
    _isDirt = true;
}

void Material::UpdateUniforms(){
    Assert(shader != nullptr);

    //if(_isDirt == false) return;
    //_isDirt = false;

    shader->Bind();

    int curTex = 0;

    for(auto i: maps){
        MaterialMap& map = i.second;

        //if(map.isDirt == false) continue;
        //map.isDirt = false;

        if(map.type == MaterialMap::Type::Float){
            shader->SetFloat(i.first.c_str(), map.value);
        }
        if(map.type == MaterialMap::Type::Vector2){
            shader->SetVector2(i.first.c_str(), Vector2(map.vector.x, map.vector.y));
        }
        if(map.type == MaterialMap::Type::Vector3){
            shader->SetVector3(i.first.c_str(), Vector3(map.vector.x, map.vector.y, map.vector.z));
        }
        if(map.type == MaterialMap::Type::Vector4){
            shader->SetVector4(i.first.c_str(), map.vector);
        }
        if(map.type == MaterialMap::Type::Texture){
            shader->SetTexture2D(i.first.c_str(), *i.second.texture, curTex);
            //i.second.texture->Bind(curTex, i.first.c_str(), *shader);
            curTex += 1;
        }
    }
}

void Material::Save(std::string& path){
    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "Shader" << YAML::Value << shader->path();

    out << YAML::Key << "Maps" << YAML::BeginSeq;

    
    for(auto i: maps){
        const std::string& name = i.first;
        MaterialMap& map = i.second;

        out << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << i.first;
        out << YAML::Key << "Type" << YAML::Value << (int)map.type;
        out << YAML::Key << "Texture" << YAML::Value << (map.texture == nullptr ? std::string() : map.texture->path());
        out << YAML::Key << "Vector" << YAML::Value << map.vector;
        out << YAML::Key << "Value" << YAML::Value << map.value;
        out << YAML::EndMap;
    }
    
    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream fout(path);
    fout << out.c_str();
}

void Material::OnGui(){
    for(auto i: maps){
        if(i.second.type == MaterialMap::Type::Float){
            ImGui::DragFloat(i.first.c_str(), &i.second.value);
        }

        if(i.second.type == MaterialMap::Type::Vector2){
            ImGui::DragFloat2(i.first.c_str(), &i.second.vector[0]);
        }

        if(i.second.type == MaterialMap::Type::Vector3){
            ImGui::DragFloat3(i.first.c_str(), &i.second.vector[0]);
        }

        if(i.second.type == MaterialMap::Type::Vector4){
            ImGui::DragFloat4(i.first.c_str(), &i.second.vector[0]);
        }

        if(i.second.type == MaterialMap::Type::Texture){
            const float widthSize = 60;
            float aspect = i.second.texture->width() / i.second.texture->height();
            ImGui::Image((void*)(uint64_t)i.second.texture->renderId(), ImVec2(widthSize, widthSize * aspect), ImVec2(0, 0), ImVec2(1, -1));
            ImGui::SameLine();
            ImGui::Text(i.first.c_str());
        }
    }
}

Ref<Material> Material::CreateFromFile(std::string const &path){
    std::ifstream stream(path);
    std::stringstream strStream;
    strStream << stream.rdbuf();

    YAML::Node data = YAML::Load(strStream.str());
    if(!data["Shader"] || !data["Maps"]){
        LogInfo("Coud not load Material: %s", path.c_str());
        return nullptr;
    }

    Ref<Material> out = CreateRef<Material>();

    std::string shaderPath = data["Shader"].as<std::string>();
    out->shader = AssetManager::Get().LoadShaderFromFile(shaderPath);

    //LogInfo("%s", shaderPath.c_str());

    for(auto i: data["Maps"]){
        MaterialMap map;
        map.type = (MaterialMap::Type)i["Type"].as<int>();
        map.value = i["Value"].as<float>();
        map.vector = i["Vector"].as<Vector4>();

        //LogInfo("%s", i["Name"].as<std::string>().c_str());
        
        std::string texturePath = i["Texture"].as<std::string>();
        if(texturePath.empty()){
            map.texture = nullptr;
        } else {
            map.texture = AssetManager::Get().LoadTexture2D(texturePath, TextureFilter::Linear, false);
        }

        out->maps[i["Name"].as<std::string>()] = map;
    }

    out->_path = path;
    return out;
}

}