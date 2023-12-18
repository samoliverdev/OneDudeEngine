#include "Material.h"
#include "Renderer.h"
#include <fstream>
#include "OD/Serialization/Serialization.h"
#include "OD/Core/AssetManager.h"
#include "OD/Core/ImGui.h"
#include <filesystem>
#include <magic_enum/magic_enum.hpp>

namespace OD{

uint32_t Material::baseId = 0;

Material::Material(){
    id = baseId;
    baseId += 1;
}

Material::Material(Ref<Shader> s){
    shader = s; 
    UpdateMaps();

    id = baseId;
    baseId += 1;
}

Ref<Shader> Material::GetShader(){ 
    return shader; 
}

void Material::SetShader(Ref<Shader> s){ 
    shader = s; 
    UpdateMaps(); 
}

uint32_t Material::MaterialId(){ 
    return id; 
}

bool Material::IsBlend(){
    if(shader == nullptr) return false;
    return shader->IsBlend();
}

bool Material::EnableInstancingValid(){ 
    return enableInstancing && SupportInstancing(); 
}

bool Material::EnableInstancing(){ 
    return enableInstancing; 
}

bool Material::SupportInstancing(){ 
    return shader != nullptr && shader->SupportInstancing(); 
}

void Material::SetFloat(const char* name, float value, float min, float max){
    MaterialMap& map = maps[name];

    if(map.type == MaterialMap::Type::Float && map.value == value) return;

    map.type = MaterialMap::Type::Float;
    map.value = value;
    map.isDirt = true;
    isDirt = true;

    map.valueMin = min;
    map.valueMax = max;
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

void Material::UpdateDatas(){
    Assert(shader != nullptr);

    //if(_isDirt == false) return;
    //_isDirt = false;

    Renderer::SetCullFace(shader->GetCullFace());
    Renderer::SetDepthTest(shader->GetDepthTest());
    Renderer::SetDepthMask(shader->IsDepthMask());

    if(shader->IsBlend()){
        Renderer::SetBlend(true);
        Renderer::SetBlendFunc(shader->GetSrcBlend(), shader->GetDstBlend());
    } else {
        Renderer::SetBlend(false);
    }

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
            if(i.second.valueMax != i.second.valueMin){
                if(ImGui::SliderFloat(i.first.c_str(), &i.second.value, i.second.valueMin, i.second.valueMax)){
                    toSave = true;
                }
            } else {
                if(ImGui::DragFloat(i.first.c_str(), &i.second.value, 1, i.second.valueMin, i.second.valueMax)){
                    toSave = true;
                }
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

    if(shader != nullptr && ImGui::TreeNode("Info")){

        std::string supportInstancing = "false";
        if(shader != nullptr && shader->SupportInstancing() == true) supportInstancing = "true";
        ImGui::Text("SupportInstancing: %s", supportInstancing.c_str());

        std::string cullFace(magic_enum::enum_name(shader->GetCullFace()));
        ImGui::Text("CullFace: %s", cullFace.c_str());

        std::string depthTest(magic_enum::enum_name(shader->GetDepthTest()));
        ImGui::Text("DepthTest: %s", depthTest.c_str());

        if(shader->IsBlend()){
            std::string srcBlend(magic_enum::enum_name(shader->GetSrcBlend()));
            std::string dstBlend(magic_enum::enum_name(shader->GetDstBlend()));
            ImGui::Text("Blend: %s %s", srcBlend.c_str(), dstBlend.c_str());
        } else {
            ImGui::Text("Blend: Off");
        }

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

    ///*
    //maps.clear();

    //Remove Unused Maps
    for(auto i: shader->Properties()){
        if(maps.count(i[1].c_str())) continue;

        maps.erase(i[1].c_str());
    }

    // Add If Not Contains
    for(auto i: shader->Properties()){
        if(i.size() < 2) continue;
        
        if(!maps.count(i[1].c_str()) && i[0] == "Float"){
            Assert(i.size() >= 3);

            if(i.size() == 3){
                SetFloat(i[1].c_str(), std::stof(i[2]));
            }

            if(i.size() == 5){
                SetFloat(
                    i[1].c_str(), 
                    std::stof(i[2]),
                    std::stof(i[3]),
                    std::stof(i[4])
                );
            }
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Texture2D"){
            SetTexture(i[1].c_str(), AssetManager::Get().LoadDefautlTexture2D());
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Color4"){
            SetVector4(i[1].c_str(), Vector4(1,1,1,1), true);
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Color3"){
            SetVector3(i[1].c_str(), Vector3(1,1,1), true);
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Vetor4"){
            SetVector4(i[1].c_str(), Vector4(1,1,1,1), false);
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Vetor3"){
            SetVector3(i[1].c_str(), Vector3(1,1,1), false);
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Vetor2"){
            SetVector2(i[1].c_str(), Vector2(1,1));
        }
    }
    //*/

}

}