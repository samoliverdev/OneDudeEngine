#include "Material.h"
#include <fstream>
#include "OD/Serialization/Serialization.h"
#include "OD/Core/AssetManager.h"
#include "OD/Core/ImGui.h"
#include <filesystem>

namespace OD{

uint32_t Material::baseId = 0;

void Material::SetFloat(const char* name, float value){
    MaterialMap& map = maps[name];

    if(map.type == MaterialMap::Type::Float && map.value == value) return;

    map.type = MaterialMap::Type::Float;
    map.value = value;
    map.isDirt = true;
    isDirt = true;
}

void Material::SetVector2(const char* name, Vector2 value){
    MaterialMap& map = maps[name];

    if(map.type == MaterialMap::Type::Vector2 && map.vector == Vector4(value.x, value.y, 0, 1)) return;

    map.type = MaterialMap::Type::Vector2;
    map.vector = Vector4(value.x, value.y, 0, 1);
    map.isDirt = true;
    isDirt = true;
}

void Material::SetVector3(const char* name, Vector3 value, bool isColor){
    MaterialMap& map = maps[name];

    if(map.type == MaterialMap::Type::Vector3 && map.vector == Vector4(value.x, value.y, value.z, 1)) return;

    map.type = MaterialMap::Type::Vector3;
    map.vector = Vector4(value.x, value.y, value.z, 1);
    map.isDirt = true;
    map.vectorIsColor = isColor;
    isDirt = true;
}

void Material::SetVector4(const char* name, Vector4 value, bool isColor){
    MaterialMap& map = maps[name];

    if(map.type == MaterialMap::Type::Vector4 &&  map.vector == value) return;

    map.type = MaterialMap::Type::Vector4;
    map.vector = value;
    map.isDirt = true;
    map.vectorIsColor = isColor;
    isDirt = true;
}

void Material::SetTexture(const char* name, Ref<Texture2D> tex){
    MaterialMap& map = maps[name];

    if(map.type == MaterialMap::Type::Texture && map.texture == tex) return;

    map.type = MaterialMap::Type::Texture;
    map.texture = tex;
    map.isDirt = true;
    isDirt = true;
}

void Material::SetCubemap(const char* name, Ref<Cubemap> tex){
    MaterialMap& map = maps[name];

    if(map.type == MaterialMap::Type::Cubemap && map.cubemap == tex) return;

    map.type = MaterialMap::Type::Cubemap;
    map.cubemap = tex;
    map.isDirt = true;
    isDirt = true;
}

void Material::UpdateUniforms(){
    Assert(shader != nullptr);

    //if(_isDirt == false) return;
    //_isDirt = false;

    Shader::Bind(*shader);

    int curTex = 0;

    for(auto i: maps){
        MaterialMap& map = i.second;

        if(map.isDirt != true) continue;
        map.isDirt = false;

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
        if(map.type == MaterialMap::Type::Cubemap){
            shader->SetCubemap(i.first.c_str(), *i.second.cubemap, curTex);
            curTex += 1;
        }
    }
}

void Material::OnGui(){
    bool toSave = false;

    ImGui::BeginGroup();
    ImGui::Text("Shader: %s", (shader == nullptr ? "" : shader->Path().c_str()));
    ImGui::EndGroup();
    ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
        if(path->string().empty() == false && path->extension() == ".glsl"){
            //_shader = AssetManager::Get().LoadShaderFromFile(path->string());
            SetShader(AssetManager::Get().LoadShaderFromFile(path->string()));
            toSave = true;
        }
    });

    ImGui::Spacing();ImGui::Spacing();

    for(auto& i: maps){
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
            float aspect = 1;

            if(i.second.texture != nullptr){
                Assert(i.second.texture->Height() != 0);
                aspect = i.second.texture->Width() / i.second.texture->Height();
            }

            ImGui::BeginGroup();
            ImVec2 imagePos = ImGui::GetCursorPos();
            ImGui::Image((void*)(uint64_t)i.second.texture->RenderId(), ImVec2(widthSize, widthSize * aspect), ImVec2(0, 0), ImVec2(1, -1));
            ImGui::SetCursorPos(imagePos);
            if(ImGui::SmallButton("X")){
                i.second.texture = AssetManager::Get().LoadDefautlTexture2D();
                toSave = true;
            }
            ImGui::EndGroup();

            ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
                if(path->string().empty() == false && (path->extension() == ".png" || path->extension() == ".jpg")){
                    i.second.texture = AssetManager::Get().LoadTexture2D(path->string(), {TextureFilter::Linear, true});
                    toSave = true;
                }
            });

            ImGui::SameLine();
            ImGui::Text(i.first.c_str());
        }
    }

    if(shader != nullptr && shader->SupportInstancing() && ImGui::Checkbox("enableInstancing", &enableInstancing)){
        toSave = true;
    }

    ImGui::Spacing();ImGui::Spacing();

    if(ImGui::TreeNode("Info")){
        std::string blendMode = "OFF";
        if(shader != nullptr && shader->GetBlendMode() == Shader::BlendMode::Blend) blendMode = "Blend";
        ImGui::Text("BlendMode: %s", blendMode.c_str());

        std::string supportInstancing = "false";
        if(shader != nullptr && shader->SupportInstancing() == true) supportInstancing = "true";
        ImGui::Text("SupportInstancing: %s", supportInstancing.c_str());

        ImGui::TreePop();
    }

    if(toSave && this->path.empty() == false) Save(this->path);
}

void Material::Save(std::string& path){
    LogInfo("Saving: %s", path.c_str());

    std::ofstream os(path);
    cereal::JSONOutputArchive archive{os};
    //archive(CEREAL_NVP(*this));
    archive(cereal::make_nvp("Material",*this));
}

Ref<Material> Material::CreateFromFile(std::string const &path){
    Ref<Material> m = CreateRef<Material>();
    m->Path(path);

    std::ifstream os(path);
    cereal::JSONInputArchive archive{os};
    archive(*m);

    return m;
}

void Material::UpdateMaps(){
    if(shader == nullptr) return;

    maps.clear();

    for(auto i: shader->Properties()){
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