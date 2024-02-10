#include "Material.h"
#include "Graphics.h"
#include <fstream>
#include "OD/Serialization/Serialization.h"
#include "OD/Core/AssetManager.h"
#include "OD/Core/ImGui.h"
#include <filesystem>
#include <magic_enum/magic_enum.hpp>

namespace OD{

uint32_t Material::baseId = 0;
std::unordered_map<std::string, MaterialMap> Material::globalMaps;

Material::Material(){
    id = baseId;
    baseId += 1;
}

Material::Material(Ref<Shader> s){
    SetShader(s);
    id = baseId;
    baseId += 1;
}

Ref<Shader> Material::GetShader(){ 
    //return shader;
    return shaderHandler->GetCurrentShader(); 
}

void Material::SetShader(Ref<Shader> s){ 
    //shader = s; 
    shaderHandler = CreateRef<ShaderHandler>(s);
    UpdateMaps(); 
}

uint32_t Material::MaterialId(){ 
    return id; 
}

bool Material::IsBlend(){
    if(GetShader() == nullptr) return false;
    return GetShader()->IsBlend();
}

bool Material::EnableInstancingValid(){ 
    return enableInstancing && SupportInstancing(); 
}

bool Material::EnableInstancing(){ 
    return enableInstancing; 
}

bool Material::SupportInstancing(){ 
    return GetShader() != nullptr && GetShader()->SupportInstancing(); 
}

void Material::SetInt(const char* name, int value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Int;
    map.valueInt = value;
}

void Material::SetFloat(const char* name, float value, float min, float max){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Float;
    map.value = value;
    map.valueMin = min;
    map.valueMax = max;
}

void Material::SetFloat(const char* name, float* value, int count){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::FloatList;
    map.list = value;
    map.listCount = count;
}

void Material::SetVector2(const char* name, Vector2 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector2;
    map.vector = Vector4(value.x, value.y, 0, 1);
}

void Material::SetVector3(const char* name, Vector3 value, bool isColor){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector3;
    map.vector = Vector4(value.x, value.y, value.z, 1);
    map.vectorIsColor = isColor;
}

void Material::SetVector4(const char* name, Vector4 value, bool isColor){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector4;
    map.vector = value;
    map.vectorIsColor = isColor;
}

void Material::SetVector4(const char* name, Vector4* value, int count){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector4List;
    map.list = value;
    map.listCount = count;
}

void Material::SetMatrix4(const char* name, Matrix4 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Matrix4;
    map.matrix = value;
}   

void Material::SetMatrix4(const char* name, Matrix4* value, int count){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Matrix4List;
    map.list = value;
    map.listCount = count;
}

void Material::SetTexture(const char* name, Ref<Texture2D> tex){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Texture;
    map.texture = tex;
}

void Material::SetTexture(const char* name, Framebuffer* tex, int bind, int attachment){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Framebuffer;
    map.framebuffer = tex;
    map.framebufferBind = bind;
    map.framebufferAttachment = attachment;
}

void Material::SetCubemap(const char* name, Ref<Cubemap> tex){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Cubemap;
    map.cubemap = tex;
}

void Material::SetGlobalInt(const char* name, int value){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Int;
    map.valueInt = value;
}

void Material::SetGlobalFloat(const char* name, float value, float min, float max){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Float;
    map.value = value;
    map.valueMin = min;
    map.valueMax = max;
}

void Material::SetGlobalFloat(const char* name, float* value, int count){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::FloatList;
    map.list = value;
    map.listCount = count;
}

void Material::SetGlobalVector2(const char* name, Vector2 value){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Vector2;
    map.vector = Vector4(value.x, value.y, 0, 1);
}

void Material::SetGlobalVector3(const char* name, Vector3 value, bool isColor){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Vector3;
    map.vector = Vector4(value.x, value.y, value.z, 1);
    map.vectorIsColor = isColor;
}

void Material::SetGlobalVector4(const char* name, Vector4 value, bool isColor){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Vector4;
    map.vector = value;
    map.vectorIsColor = isColor;
}

void Material::SetGlobalVector4(const char* name, Vector4* value, int count){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Vector4List;
    map.list = value;
    map.listCount = count;
}

void Material::SetGlobalMatrix4(const char* name, Matrix4 value){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Matrix4;
    map.matrix = value;
}   

void Material::SetGlobalMatrix4(const char* name, Matrix4* value, int count){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Matrix4List;
    map.list = value;
    map.listCount = count;
}

void Material::SetGlobalTexture(const char* name, Ref<Texture2D> tex){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Texture;
    map.texture = tex;
}

void Material::SetGlobalTexture(const char* name, Framebuffer* tex, int bind, int attachment){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Framebuffer;
    map.framebuffer = tex;
    map.framebufferBind = bind;
    map.framebufferAttachment = attachment;
}

void Material::SetGlobalCubemap(const char* name, Ref<Cubemap> tex){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Cubemap;
    map.cubemap = tex;
}

void Material::DisableKeyword(std::string keyword){
    shaderHandler->DisableKeyword(keyword);
}

void Material::EnableKeyword(std::string keyword){
    shaderHandler->EnableKeyword(keyword);
}

void Material::CleanData(){
    maps.clear();
}

/*
void Material::UpdateDatas(){
    Ref<Shader> shader = GetShader();

    Assert(shader != nullptr);

    //if(_isDirt == false) return;
    //_isDirt = false;

    Graphics::SetCullFace(shader->GetCullFace());
    Graphics::SetDepthTest(shader->GetDepthTest());
    Graphics::SetDepthMask(shader->IsDepthMask());

    if(shader->IsBlend()){
        Graphics::SetBlend(true);
        Graphics::SetBlendFunc(shader->GetSrcBlend(), shader->GetDstBlend());
    } else {
        Graphics::SetBlend(false);
    }

    ApplyUniformTo(*shader);
}
*/

void Material::SubmitGraphicDatas(Material& material){
    material.currentTextureSlot = 0;
    material.shaderHandler->SetCurrentShader();

    Assert(material.GetShader() != nullptr);
    if(material.GetShader() == nullptr) return;

    Graphics::SetCullFace(material.GetShader()->GetCullFace());
    Graphics::SetDepthTest(material.GetShader()->GetDepthTest());
    Graphics::SetDepthMask(material.GetShader()->IsDepthMask());

    if(material.GetShader()->IsBlend()){
        Graphics::SetBlend(true);
        Graphics::SetBlendFunc(material.GetShader()->GetSrcBlend(), material.GetShader()->GetDstBlend());
    } else {
        Graphics::SetBlend(false);
    }

    Assert(material.currentTextureSlot < 50);
    ApplyUniformTo(material, *material.GetShader(), material.maps);
    ApplyUniformTo(material, *material.GetShader(), globalMaps);

}

void Material::CleanGlobalUniformsData(){
    globalMaps.clear();
}

void Material::OnGui(){
    bool toSave = false;

    //ImGui::BeginDisabled();

    ImGui::BeginGroup();
    ImGui::Text("Shader: %s", (GetShader() == nullptr ? "" : GetShader()->Path().c_str()));
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

    if(GetShader() != nullptr && GetShader()->SupportInstancing() && ImGui::Checkbox("enableInstancing", &enableInstancing)){
        toSave = true;
    }

    ImGui::Spacing();ImGui::Spacing();

    if(GetShader() != nullptr && ImGui::TreeNode("Info")){

        std::string supportInstancing = "false";
        if(GetShader() != nullptr && GetShader()->SupportInstancing() == true) supportInstancing = "true";
        ImGui::Text("SupportInstancing: %s", supportInstancing.c_str());

        std::string cullFace(magic_enum::enum_name(GetShader()->GetCullFace()));
        ImGui::Text("CullFace: %s", cullFace.c_str());

        std::string depthTest(magic_enum::enum_name(GetShader()->GetDepthTest()));
        ImGui::Text("DepthTest: %s", depthTest.c_str());

        if(GetShader()->IsBlend()){
            std::string srcBlend(magic_enum::enum_name(GetShader()->GetSrcBlend()));
            std::string dstBlend(magic_enum::enum_name(GetShader()->GetDstBlend()));
            ImGui::Text("Blend: %s %s", srcBlend.c_str(), dstBlend.c_str());
        } else {
            ImGui::Text("Blend: Off");
        }

        ImGui::TreePop();
    }

    //ImGui::EndDisabled();

    if(toSave && this->path.empty() == false) Save(this->path);
}

void Material::Save(std::string& path){
    return;
    Assert(false && "Not Implemented");

    LogInfo("Saving: %s", path.c_str());

    std::ofstream os(path);
    cereal::JSONOutputArchive archive{os};
    //archive(CEREAL_NVP(*this));
    archive(cereal::make_nvp("Material",*this));
}

Ref<Material> Material::CreateFromFile(std::string const &path){
    Assert(false && "Not Implemented");

    Ref<Material> m = CreateRef<Material>();
    m->Path(path);

    std::ifstream os(path);
    cereal::JSONInputArchive archive{os};
    archive(*m);

    return m;
}

void Material::UpdateMaps(){
    if(GetShader() == nullptr) return;

    ///*
    //maps.clear();

    //Remove Unused Maps
    for(auto i: GetShader()->Properties()){
        if(maps.count(i[1].c_str())) continue;

        maps.erase(i[1].c_str());
    }

    // Add If Not Contains
    for(auto i: GetShader()->Properties()){
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

void Material::ApplyUniformTo(Material& material, Shader& shader, std::unordered_map<std::string, MaterialMap>& maps){
    Shader::Bind(shader);

    for(auto i: maps){
        MaterialMap& map = i.second;

        if(shader.ContainUniformName(i.first) == false){
            continue;
        }

        if(map.type == MaterialMap::Type::Int){
            shader.SetInt(i.first.c_str(), map.valueInt);
        }
        if(map.type == MaterialMap::Type::Float){
            shader.SetFloat(i.first.c_str(), map.value);
        }
        if(map.type == MaterialMap::Type::Vector2){
            shader.SetVector2(i.first.c_str(), Vector2(map.vector.x, map.vector.y));
        }
        if(map.type == MaterialMap::Type::Vector3){
            shader.SetVector3(i.first.c_str(), Vector3(map.vector.x, map.vector.y, map.vector.z));
        }
        if(map.type == MaterialMap::Type::Vector4){
            shader.SetVector4(i.first.c_str(), map.vector);
        }
        if(map.type == MaterialMap::Type::Matrix4){
            shader.SetMatrix4(i.first.c_str(), i.second.matrix);
        }
        if(map.type == MaterialMap::Type::Texture){
            shader.SetTexture2D(i.first.c_str(), *i.second.texture, material.currentTextureSlot);
            material.currentTextureSlot += 1;
        }
        if(map.type == MaterialMap::Type::Framebuffer){
            shader.SetFramebuffer(i.first.c_str(), *i.second.framebuffer, material.currentTextureSlot, map.framebufferAttachment);
            material.currentTextureSlot += 1;
        }
        if(map.type == MaterialMap::Type::Cubemap){
            shader.SetCubemap(i.first.c_str(), *i.second.cubemap, material.currentTextureSlot);
            material.currentTextureSlot += 1;
        }
        if(map.type == MaterialMap::Type::FloatList){
            shader.SetFloat(i.first.c_str(), static_cast<float*>(map.list), map.listCount);
        }
        if(map.type == MaterialMap::Type::Vector4List){
            shader.SetVector4(i.first.c_str(), static_cast<Vector4*>(map.list), map.listCount);
        }
        if(map.type == MaterialMap::Type::Matrix4List){
            shader.SetMatrix4(i.first.c_str(), static_cast<Matrix4*>(map.list), map.listCount);
        }
    }
}

}