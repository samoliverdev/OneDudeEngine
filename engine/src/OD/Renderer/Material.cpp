#include "Material.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include "OD/Serialization/Serialization.h"
#include "OD/Core/AssetManager.h"

namespace OD{

void Material::SetFloat(const char* name, float value){
    maps[name].type = MaterialMap::Type::Float;
    maps[name].value = value;
}

void Material::SetVector2(const char* name, Vector2 value){
    maps[name].type = MaterialMap::Type::Vector2;
    maps[name].vector = Vector4(value.x, value.y, 0, 1);
}

void Material::SetVector3(const char* name, Vector3 value){
    maps[name].type = MaterialMap::Type::Vector3;
    maps[name].vector = Vector4(value.x, value.y, value.z, 1);
}

void Material::SetVector4(const char* name, Vector4 value){
    maps[name].type = MaterialMap::Type::Vector4;
    maps[name].vector = value;
}

void Material::SetTexture(const char* name, Ref<Texture2D> tex){
    maps[name].type = MaterialMap::Type::Texture;
    maps[name].texture = tex;
}

void Material::UpdateUniforms(){
    Assert(shader != nullptr);

    shader->Bind();

    int curTex = 0;

    for(auto i: maps){
        MaterialMap& map = i.second;

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