#include "Material.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include "OD/Serialization/Serialization.h"
#include "OD/Core/AssetManager.h"
#include "OD/Core/ImGui.h"
#include <filesystem>

namespace OD{

void Material::SetFloat(const char* name, float value){
    MaterialMap& map = _maps[name];

    //if(map.value == value) return;

    map.type = MaterialMap::Type::Float;
    map.value = value;
    map.isDirt = true;
    _isDirt = true;
}

void Material::SetVector2(const char* name, Vector2 value){
    MaterialMap& map = _maps[name];

    //if(map.vector == Vector4(value.x, value.y, 0, 1)) return;

    map.type = MaterialMap::Type::Vector2;
    map.vector = Vector4(value.x, value.y, 0, 1);
    map.isDirt = true;
    _isDirt = true;
}

void Material::SetVector3(const char* name, Vector3 value, bool isColor){
    MaterialMap& map = _maps[name];

    //if(map.vector == Vector4(value.x, value.y, value.z, 1)) return;

    map.type = MaterialMap::Type::Vector3;
    map.vector = Vector4(value.x, value.y, value.z, 1);
    map.isDirt = true;
    map.vectorIsColor = isColor;
    _isDirt = true;
}

void Material::SetVector4(const char* name, Vector4 value, bool isColor){
    MaterialMap& map = _maps[name];

    //if(map.vector == value) return;

    map.type = MaterialMap::Type::Vector4;
    map.vector = value;
    map.isDirt = true;
    map.vectorIsColor = isColor;
    _isDirt = true;
}

void Material::SetTexture(const char* name, Ref<Texture2D> tex){
    MaterialMap& map = _maps[name];

    //if(map.texture == tex) return;

    map.type = MaterialMap::Type::Texture;
    map.texture = tex;
    map.isDirt = true;
    _isDirt = true;
}

void Material::UpdateUniforms(){
    Assert(_shader != nullptr);

    //if(_isDirt == false) return;
    //_isDirt = false;

    _shader->Bind();

    int curTex = 0;

    for(auto i: _maps){
        MaterialMap& map = i.second;

        //if(map.isDirt == false) continue;
        //map.isDirt = false;

        if(map.type == MaterialMap::Type::Float){
            _shader->SetFloat(i.first.c_str(), map.value);
        }
        if(map.type == MaterialMap::Type::Vector2){
            _shader->SetVector2(i.first.c_str(), Vector2(map.vector.x, map.vector.y));
        }
        if(map.type == MaterialMap::Type::Vector3){
            _shader->SetVector3(i.first.c_str(), Vector3(map.vector.x, map.vector.y, map.vector.z));
        }
        if(map.type == MaterialMap::Type::Vector4){
            _shader->SetVector4(i.first.c_str(), map.vector);
        }
        if(map.type == MaterialMap::Type::Texture){
            _shader->SetTexture2D(i.first.c_str(), *i.second.texture, curTex);
            //i.second.texture->Bind(curTex, i.first.c_str(), *shader);
            curTex += 1;
        }
    }
}

void Material::OnGui(){
    bool toSave = false;

    ImGui::BeginGroup();
    ImGui::Text("Shader: %s", (_shader == nullptr ? "" : _shader->path().c_str()));
    ImGui::EndGroup();
    ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
        if(path->string().empty() == false && path->extension() == ".glsl"){
            //_shader = AssetManager::Get().LoadShaderFromFile(path->string());
            shader(AssetManager::Get().LoadShaderFromFile(path->string()));
            toSave = true;
        }
    });

    ImGui::Spacing();ImGui::Spacing();

    for(auto& i: _maps){
        if(i.second.type == MaterialMap::Type::Float){
            if(ImGui::DragFloat(i.first.c_str(), &i.second.value)){
                toSave = true;
            }
        }

        if(i.second.type == MaterialMap::Type::Vector2){
            if(ImGui::DragFloat2(i.first.c_str(), &i.second.vector[0])){
                toSave = true;
            }
        }
        
        if(i.second.type == MaterialMap::Type::Vector3 && i.second.vectorIsColor == false){
            if(ImGui::DragFloat3(i.first.c_str(), &i.second.vector[0])){
                toSave = true;
            }
        }

        if(i.second.type == MaterialMap::Type::Vector4 && i.second.vectorIsColor == false){
            if(ImGui::DragFloat4(i.first.c_str(), &i.second.vector[0])){
                toSave = true;
            }
        }

        if(i.second.type == MaterialMap::Type::Vector3 && i.second.vectorIsColor == true){
            if(ImGui::ColorEdit3(i.first.c_str(), &i.second.vector[0])){
                toSave = true;
            }
        }

        if(i.second.type == MaterialMap::Type::Vector4 && i.second.vectorIsColor == true){
            if(ImGui::ColorEdit4(i.first.c_str(), &i.second.vector[0])){
                toSave = true;
            }
        }

        if(i.second.type == MaterialMap::Type::Texture){
            const float widthSize = 60;
            float aspect = i.second.texture->width() / i.second.texture->height();

            ImGui::BeginGroup();
            ImVec2 imagePos = ImGui::GetCursorPos();
            ImGui::Image((void*)(uint64_t)i.second.texture->renderId(), ImVec2(widthSize, widthSize * aspect), ImVec2(0, 0), ImVec2(1, -1));
            ImGui::SetCursorPos(imagePos);
            if(ImGui::SmallButton("X")){
                i.second.texture = AssetManager::Get().LoadDefautlTexture2D();
                toSave = true;
            }
            ImGui::EndGroup();

            ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
                if(path->string().empty() == false && (path->extension() == ".png" || path->extension() == ".jpg")){
                    i.second.texture = AssetManager::Get().LoadTexture2D(path->string(), TextureFilter::Linear, true);
                    toSave = true;
                }
            });

            ImGui::SameLine();
            ImGui::Text(i.first.c_str());
        }
    }

    ImGui::Spacing();ImGui::Spacing();

    if(ImGui::TreeNode("Info")){
        std::string blendMode = "OFF";
        if(_shader != nullptr && _shader->blendMode() == Shader::BlendMode::Blend) blendMode = "Blend";
        ImGui::Text("BlendMode: %s", blendMode.c_str());

        std::string supportInstancing = "false";
        if(_shader != nullptr && _shader->supportInstancing() == true) supportInstancing = "true";
        ImGui::Text("SupportInstancing: %s", supportInstancing);

        ImGui::TreePop();
    }

    if(toSave && _path.empty() == false) Save(_path);
}

void Material::Save(std::string& path){
    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "Shader" << YAML::Value << _shader->path();
    out << YAML::Key << "Maps" << YAML::BeginSeq;

    for(auto i: _maps){
        const std::string& name = i.first;
        MaterialMap& map = i.second;

        out << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << i.first;
        out << YAML::Key << "Type" << YAML::Value << (int)map.type;
        out << YAML::Key << "Texture" << YAML::Value << (map.texture == nullptr ? std::string() : map.texture->path());
        out << YAML::Key << "Vector" << YAML::Value << map.vector;
        out << YAML::Key << "Value" << YAML::Value << map.value;
        out << YAML::Key << "vectorIsColor" << YAML::Value << map.vectorIsColor;
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
    out->_shader = AssetManager::Get().LoadShaderFromFile(shaderPath);

    //LogInfo("%s", shaderPath.c_str());

    for(auto i: data["Maps"]){
        MaterialMap map;
        map.type = (MaterialMap::Type)i["Type"].as<int>();
        map.value = i["Value"].as<float>();
        map.vector = i["Vector"].as<Vector4>();
        if(i["vectorIsColor"]) map.vectorIsColor = i["vectorIsColor"].as<bool>();

        //LogInfo("%s", i["Name"].as<std::string>().c_str());
        
        std::string texturePath = i["Texture"].as<std::string>();
        if(texturePath.empty()){
            map.texture = nullptr;
        } else {
            map.texture = AssetManager::Get().LoadTexture2D(texturePath, TextureFilter::Linear, false);
        }

        out->_maps[i["Name"].as<std::string>()] = map;
    }

    out->_path = path;
    return out;
}

void Material::UpdateMaps(){
    if(_shader == nullptr) return;

    _maps.clear();

    for(auto i: _shader->properties()){
        if(i.size() < 2) continue;
        
        if(i[0] == "Float"){
            SetFloat(i[1].c_str(), (i.size() > 2 ? std::stof(i[2]) : 1));
        }

        if(i[0] == "Texture2D"){
            SetTexture(i[1].c_str(), AssetManager::Get().LoadDefautlTexture2D());
        }

        if(i[0] == "Color4"){
            SetVector4(i[1].c_str(), Vector4(1,1,1,1), true);
        }

        if(i[0] == "Color3"){
            SetVector3(i[1].c_str(), Vector3(1,1,1), true);
        }

        if(i[0] == "Vetor4"){
            SetVector4(i[1].c_str(), Vector4(1,1,1,1), false);
        }

        if(i[0] == "Vetor3"){
            SetVector3(i[1].c_str(), Vector3(1,1,1), false);
        }

        if(i[0] == "Vetor2"){
            SetVector2(i[1].c_str(), Vector2(1,1));
        }
    }

}

}