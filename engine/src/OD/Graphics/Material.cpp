#include "Material.h"
#include "Graphics.h"
#include <fstream>
#include "OD/Utils/PlatformUtils.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/Asset.h"
#include "OD/Core/ImGui.h"
#include <filesystem>
#include <magic_enum/magic_enum.hpp>

namespace OD{

void MaterialMap::OnLoad(std::string& texPath){
    if(texPath.empty() == false){
        if(type == MaterialMap::Type::Texture) texture = AssetManager::Get().LoadAsset<Texture2D>(texPath);
    }
}

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
    shaderHandler = CreateRef<MultiCompileShader>(s);
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
    map.valueFloat = value;
    map.valueFloatMin = min;
    map.valueFloatMax = max;
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

void Material::SetVector3(const char* name, Vector3 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector3;
    map.vector = Vector4(value.x, value.y, value.z, 1);
}

void Material::SetVector4(const char* name, Vector4 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector4;
    map.vector = value;
}

void Material::SetColor3(const char* name, Vector3 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector3;
    map.vector = Vector4(value.x, value.y, value.z, 1);
    map.vectorIsColor = true;
}

void Material::SetColor4(const char* name, Vector4 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector4;
    map.vector = value;
    map.vectorIsColor = true;
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

void Material::SetTexture(const char* name, Ref<Texture2DArray> tex){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::TextureArray;
    map.textureArray = tex;
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
    map.valueFloat = value;
    map.valueFloatMin = min;
    map.valueFloatMax = max;
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

void Material::SetGlobalVector3(const char* name, Vector3 value){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Vector3;
    map.vector = Vector4(value.x, value.y, value.z, 1);
}

void Material::SetGlobalVector4(const char* name, Vector4 value){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Vector4;
    map.vector = value;
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
            //SetShader(AssetManager::Get().LoadShaderFromFile(path->string()));
            SetShader(AssetManager::Get().LoadAsset<Shader>(path->string()));
            toSave = true;
        }
    });

    ImGui::Spacing();ImGui::Spacing();

    /*for(auto& i: maps){
        const std::string& name = i.first;
        MaterialMap& map = i.second;*/

    for(auto& i: properties){
        const std::string& name = i;
        MaterialMap& map = maps[i];

        if(map.type == MaterialMap::Type::Float){
            if(map.valueFloatMax != map.valueFloatMin){
                if(ImGui::SliderFloat(name.c_str(), &map.valueFloat, map.valueFloatMin, map.valueFloatMax)){
                    toSave = true;
                }
            } else {
                if(ImGui::DragFloat(name.c_str(), &map.valueFloat, 1, map.valueFloatMin, map.valueFloatMax)){
                    toSave = true;
                }
            }
        }

        if(map.type == MaterialMap::Type::Vector2){
            if(ImGui::DragFloat2(name.c_str(), &map.vector[0])){
                toSave = true;
            }
        }
        
        if(map.type == MaterialMap::Type::Vector3 && map.vectorIsColor == false){
            if(ImGui::DragFloat3(name.c_str(), &map.vector[0])){
                toSave = true;
            }
        }

        if(map.type == MaterialMap::Type::Vector4 && map.vectorIsColor == false){
            if(ImGui::DragFloat4(name.c_str(), &map.vector[0])){
                toSave = true;
            }
        }

        if(map.type == MaterialMap::Type::Vector3 && map.vectorIsColor == true){
            if(ImGui::ColorEdit3(name.c_str(), &map.vector[0])){
                toSave = true;
            }
        }

        if(map.type == MaterialMap::Type::Vector4 && map.vectorIsColor == true){
            if(ImGui::ColorEdit4(name.c_str(), &map.vector[0]/*, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR*/)){
                toSave = true;
            }
        }

        if(map.type == MaterialMap::Type::Texture){
            const float widthSize = 60;
            float aspect = 1;

            if(map.texture != nullptr){
                Assert(map.texture->Height() != 0);
                aspect = map.texture->Width() / map.texture->Height();
            }

            ImGui::BeginGroup();
            ImVec2 imagePos = ImGui::GetCursorPos();
            ImGui::Image((void*)(uint64_t)map.texture->RenderId(), ImVec2(widthSize, widthSize * aspect), ImVec2(0, 0), ImVec2(1, -1));
            ImGui::SetCursorPos(imagePos);
            if(ImGui::SmallButton("X")){
                map.texture = Texture2D::LoadDefautlTexture2D();
                toSave = true;
            }
            ImGui::EndGroup();

            ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
                if(path->string().empty() == false && (path->extension() == ".png" || path->extension() == ".jpg")){
                    map.texture = AssetManager::Get().LoadAsset<Texture2D>(path->string());
                    toSave = true;
                }
            });

            ImGui::SameLine();
            ImGui::Text(name.c_str());
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

    if(this->path.empty() == true || this->path == "Memory"){
        if(ImGui::Button("Save As")){
            std::string _path = FileDialogs::SaveFile("*.material");
            if(_path.empty() == false){
                Save(_path);
            } 
        }
    }
}

void Material::Save(std::string& path){
    //return;
    //Assert(false && "Not Implemented");

    LogInfo("Saving: %s", path.c_str());

    std::ofstream os(path);
    cereal::JSONOutputArchive archive{os};
    //archive(CEREAL_NVP(*this));
    archive(cereal::make_nvp("Material",*this));
}

bool Material::LoadFromFile(const std::string& inPath){
    path = inPath;

    try{
        std::ifstream os(inPath);
        cereal::JSONInputArchive archive{os};
        archive(*this);
    } catch(...){
        return false;
    }

    return true;
}

std::vector<std::string> Material::GetFileAssociations(){
    return std::vector<std::string>{
		".material"
	};
}

/*Ref<Material> Material::CreateFromFile(std::string const &path){
    //Assert(false && "Not Implemented");

    Ref<Material> m = CreateRef<Material>();
    m->Path(path);

    std::ifstream os(path);
    cereal::JSONInputArchive archive{os};
    archive(*m);

    return m;
}*/

void Material::UpdateMaps(){
    if(GetShader() == nullptr) return;

    ///*

    //Remove Unused Maps
    for(auto i: GetShader()->Properties()){
        if(maps.count(i[1].c_str())) continue;
        maps.erase(i[1].c_str());
    }
    //maps.clear();
    properties.clear();

    // Add If Not Contains
    for(auto i: GetShader()->Properties()){
        if(i.size() < 2) continue;

        properties.push_back(i[1]);
        
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
            if(i[2] == "White"){
                SetTexture(i[1].c_str(), AssetManager::Get().LoadAsset<Texture2D>("res/Engine/Textures/White.jpg") );
            } else if(i[2] == "Black"){
                SetTexture(i[1].c_str(), AssetManager::Get().LoadAsset<Texture2D>("res/Engine/Textures/Black.jpg") );
            } else if(i[2] == "Normal"){
                SetTexture(i[1].c_str(), AssetManager::Get().LoadAsset<Texture2D>("res/Engine/Textures/Normal.jpg") );
            } else {
                SetTexture(i[1].c_str(), Texture2D::LoadDefautlTexture2D());
            }
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Color4"){
            SetColor4(i[1].c_str(), Vector4(1,1,1,1));
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Color3"){
            SetColor3(i[1].c_str(), Vector3(1,1,1));
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Vetor4"){
            SetVector4(i[1].c_str(), Vector4(1,1,1,1));
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Vetor3"){
            SetVector3(i[1].c_str(), Vector3(1,1,1));
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

        //if(shader.ContainUniformName(i.first) == false) continue;

        if(map.type == MaterialMap::Type::Int){
            shader.SetInt(i.first.c_str(), map.valueInt);
        }
        if(map.type == MaterialMap::Type::Float){
            shader.SetFloat(i.first.c_str(), map.valueFloat);
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
        if(map.type == MaterialMap::Type::TextureArray){
            shader.SetTexture2DArray(i.first.c_str(), *i.second.textureArray, material.currentTextureSlot);
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