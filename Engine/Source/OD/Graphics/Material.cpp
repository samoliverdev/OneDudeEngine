#include "OD/pch.h"
#include "Material.h"
#include "Common.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "Texture.h"
#include "OD/Platform/Platform.h"
#include "OD/Serialization/SerializationFull.h"
#include "OD/Core/Asset.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Lua.h"

namespace OD{

/*extern*/ GraphicsStats stats;
extern GraphicsDevice* graphicsDevice;

void MaterialMap::OnLoad(std::string& texPath){
    if(texPath.empty() == false){
        if(type == MaterialMap::Type::Texture) texture = AssetManager::Get().LoadAsset<Texture2D>(texPath);
    }
}

std::unordered_map<std::string, MaterialMap> Material::globalMaps{};
IdPool materialIdPool;

Material::Material(){
    id = materialIdPool.Pop();
}

Material::Material(const std::string& label){
    id = materialIdPool.Pop();
    path = "#" + label;
}

Material::Material(Ref<Shader> s, bool inenableInstancing){
    SetShader(s);
    id = materialIdPool.Pop();
    SetEnableInstancing(inenableInstancing);
}

//TODO: This can be have some bug by copy everything, need test this later
Material::Material(const Material& other){
    maps = other.maps;
    path = "#Memory";

    if(other.shader != nullptr) SetShader(other.shader);
    id = materialIdPool.Pop();
    SetEnableInstancing(other.enableInstancing);
}

Material::~Material(){
    Assert(graphicsDevice != nullptr);
    graphicsDevice->MaterialDestroy(*this);
    materialIdPool.Push(id);
}

Ref<Shader> Material::GetShader(){ 
    return shader;
}

void Material::SetShader(Ref<Shader> s){ 
    isDirty = true;
    isDirtyUniformData = true;
    shader = s; 
    //graphicsDevice->MaterialOnSetShader(*this);
    UpdateCurrentShader();
    UpdateMaps(); 

    for(int i = 0; i < shader->passes.size(); i++){
        if(shader->passes[i].name == "MainPass"){
            mainPass = i;
        }
        if(shader->passes[i].name == "DepthPass"){
            depthPass = i;
        }
    }

    graphicsDevice->MaterialOnSetShader(*this);
}

bool Material::IsBlend(){
    if(currentShader.drawTypes[0] == nullptr) return false;
    return currentShader.drawTypes[0]->IsBlend();
}

bool Material::EnableInstancingValid(){ 
    return enableInstancing && SupportInstancing(); 
}

bool Material::EnableInstancing(){ 
    return enableInstancing; 
}

bool Material::SupportInstancing(){ 
    //return false;
    return currentShader.drawTypes[0] != nullptr && currentShader.drawTypes[0]->pipeline.supportInstancing; 
}

void Material::SetInt(const char* name, int value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Int;
    map.valueInt = value;
    isDirtyUniformData = true;
}

void Material::SetFloat(const char* name, float value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Float;
    map.valueFloat = value;
    isDirtyUniformData = true;
}

void Material::SetFloat(const char* name, float value, float min, float max){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Float;
    map.valueFloat = value;
    map.valueFloatMin = min;
    map.valueFloatMax = max;
    isDirtyUniformData = true;
}

void Material::SetFloat(const char* name, float* value, int count){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::FloatList;
    map.list = value;
    map.listCount = count;
    isDirtyUniformData = true;
}

void Material::SetVector2(const char* name, Vector2 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector2;
    map.vec.vector = Vector4(value.x, value.y, 0, 1);
    isDirtyUniformData = true;
}

void Material::SetVector3(const char* name, Vector3 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector3;
    map.vec.vector = Vector4(value.x, value.y, value.z, 1);
    isDirtyUniformData = true;
}

void Material::SetVector4(const char* name, Vector4 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector4;
    map.vec.vector = value;
    isDirtyUniformData = true;
}

void Material::SetColor3(const char* name, Vector3 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector3;
    map.vec.vector = ToLinear(Vector4(value.x, value.y, value.z, 1));
    map.vec.vectorIsColor = true;
    isDirtyUniformData = true;
}

void Material::SetColor4(const char* name, Vector4 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector4;
    map.vec.vector = ToLinear(value);
    map.vec.vectorIsColor = true;
    isDirtyUniformData = true;
}

void Material::SetVector4(const char* name, Vector4* value, int count){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Vector4List;
    map.list = value;
    map.listCount = count;
    isDirtyUniformData = true;
}

void Material::SetMatrix4(const char* name, Matrix4 value){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Matrix4;
    map.matrix = value;
    isDirtyUniformData = true;
}   

void Material::SetMatrix4(const char* name, Matrix4* value, int count){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Matrix4List;
    map.list = value;
    map.listCount = count;
    isDirtyUniformData = true;
}

void Material::SetTexture(const char* name, Ref<Texture2D> tex){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Texture;
    map.texture = tex;
    isDirty = true;
}

void Material::SetTexture(const char* name, Ref<Texture2DArray> tex){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::TextureArray;
    map.textureArray = tex;
    isDirty = true;
}

void Material::SetTexture(const char* name, Framebuffer* tex, int attachment){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Framebuffer;
    map.framebuffer = tex;
    map.framebufferAttachment = attachment;
    isDirty = true;
}

void Material::SetCubemap(const char* name, Ref<Cubemap> tex){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Cubemap;
    map.cubemap = tex;
    isDirty = true;
}

void Material::SetUniformBuffer(const char* name, Ref<UniformBuffer> buffer, int bind){
    MaterialMap& map = maps[name];
    map.type = MaterialMap::Type::Buffer;
    map.buffer = buffer;
    map.uniformBufferBind = bind;
    isDirty = true;
}

void Material::SetGlobalUniformBuffer(const char* name, Ref<UniformBuffer> buffer, int bind){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Buffer;
    map.buffer = buffer;
    map.uniformBufferBind = bind;
}

void Material::SetGlobalInt(const char* name, int value){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Int;
    map.valueInt = value;
}

void Material::SetGlobalFloat(const char* name, float value/*, float min, float max*/){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Float;
    map.valueFloat = value;
    //map.valueFloatMin = min;
    //map.valueFloatMax = max;
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
    map.vec.vector = Vector4(value.x, value.y, 0, 1);
}

void Material::SetGlobalVector3(const char* name, Vector3 value){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Vector3;
    map.vec.vector = Vector4(value.x, value.y, value.z, 1);
}

void Material::SetGlobalVector4(const char* name, Vector4 value){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Vector4;
    map.vec.vector = value;
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

void Material::SetGlobalTexture(const char* name, Framebuffer* tex, int attachment){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Framebuffer;
    map.framebuffer = tex;
    map.framebufferAttachment = attachment;
}

void Material::SetGlobalCubemap(const char* name, Ref<Cubemap> tex){
    MaterialMap& map = globalMaps[name];
    map.type = MaterialMap::Type::Cubemap;
    map.cubemap = tex;
}

void Material::DisableKeyword(const std::string& keyword){
    if(shader == nullptr) return;

    for(auto& i: shader->keyworldSpaces){
        for(auto& j: i.keyworlds){
            if(j == keyword){
                i.enabledKey = -1;
                break;
            }
        }
    }

    isDirty = true;
    //isDirtyUniformData = true; //INFO: for now, i think dont need
}

void Material::EnableKeyword(const std::string& keyword){
    if(shader == nullptr) return;

    for(auto& i: shader->keyworldSpaces){
        int index = 0;
        for(auto& j: i.keyworlds){
            if(j == keyword){
                i.enabledKey = index;
                break;
            }
            index += 1;
        }
    }

    isDirty = true;
    //isDirtyUniformData = true; //INFO: for now, i think dont need
}

std::set<std::string> Material::GetEnabledKeywords(){
    //return enabledKeywords;

    std::set<std::string> out;
    for(auto i: shader->keyworldSpaces){
        if(i.enabledKey < 0){
            out.insert("");
        } else {
            out.insert(i.keyworlds[i.enabledKey]);
        }
    }
    return out;
}

void Material::SetPass(int i){
    currentPass = i;
    UpdateCurrentShader();
}

int Material::GetPass(){
    return currentPass;
}

int Material::PassCount(){
    if(shader == nullptr) return 0;
    return shader->passes.size();
}

const std::vector<std::string>& Material::TagsString(){
    return shader->passes[0].tagsString;
}

const std::vector<uint32_t>& Material::TagsHash(){
    return shader->passes[0].tagsHash;
}

std::string Material::GetKey(const std::set<std::string>& keyworlds){
    if(keyworlds.size() == 0) return "";
    return std::accumulate(keyworlds.begin(), keyworlds.end(), std::string(""));
}

void Material::UpdateCurrentShader(){
    std::string key = GetKey(GetEnabledKeywords());
    //LogInfo("Key: %s", key.c_str());

    Assert(shader->passes.size() > 0 && "Fixme");

    if(shader->passes[currentPass].shaders.count(key)){
        currentShader = shader->passes[currentPass].shaders[key];
    } else {
        LogError("No Key: {}", key);
        Assert(false);
    }
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

//void Material::SubmitGraphicDatas(Material& material){
//    stats.materialSubmitDatas += 1;

//    material.currentTextureSlot = 0;
//    material.UpdateCurrentShader();

//    Assert(material.GetShader() != nullptr);
//    if(material.GetShader() == nullptr) return;

    /*Graphics::SetColorMask(material.currentShader->pipeline.colorMask);
    Graphics::SetCullFace(material.currentShader->GetCullFace());
    Graphics::SetDepthTest(material.currentShader->GetDepthTest());
    Graphics::SetDepthMask(material.currentShader->IsDepthMask());
    if(material.currentShader->IsBlend()){
        Graphics::SetBlend(true);
        Graphics::SetBlendFunc(material.currentShader->GetSrcBlend(), material.currentShader->GetDstBlend());
    } else {
        Graphics::SetBlend(false);
    }*/

//    Graphics::Device().SubShaderBind(*material.currentShader);
    //SubShader::Bind(*material.currentShader);
//    ApplyUniformTo(material, *material.currentShader, material.maps);
//    ApplyUniformTo(material, *material.currentShader, globalMaps);
//    Assert(material.currentTextureSlot < 32);
//}

void Material::CleanGlobalUniformsData(){
    globalMaps.clear();
}

void Material::OnGui(){
    bool toSave = false;

    //ImGui::BeginDisabled();

    /*Ref<Shader> tempShader = ImGui::DrawAssetExtra<Shader>(std::string("shader"), GetShader());
    if(tempShader != nullptr){
        SetShader(tempShader);
        toSave = true;
    }*/

    ImGui::LabelText("Id", "%d", id);

    Ref<Shader> tempShader = shader;
    std::string s("shader");
    if(ImGui::DrawAsset<Shader>(s, tempShader, nullptr) && tempShader != shader){
        SetShader(tempShader);
        toSave = true;
        isDirty = isDirtyUniformData = true;
    }

    /*ImGui::BeginGroup();
    ImGui::Text("Shader: %s", (GetShader() == nullptr ? "" : GetShader()->Path().c_str()));
    ImGui::EndGroup();
    ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
        if(path->string().empty() == false && path->extension() == ".glsl"){
            //_shader = AssetManager::Get().LoadShaderFromFile(path->string());
            //SetShader(AssetManager::Get().LoadShaderFromFile(path->string()));
            SetShader(AssetManager::Get().LoadAsset<Shader>(path->string()));
            toSave = true;
        }
    });*/

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
                    isDirty = isDirtyUniformData = true;
                }
            } else {
                if(ImGui::DragFloat(name.c_str(), &map.valueFloat, 1/*, map.valueFloatMin, map.valueFloatMax*/)){
                    toSave = true;
                    isDirty = isDirtyUniformData = true;
                }
            }
        }

        if(map.type == MaterialMap::Type::Vector2){
            if(ImGui::DragFloat2(name.c_str(), &map.vec.vector[0])){
                toSave = true;
                isDirty = isDirtyUniformData = true;
            }
        }
        
        if(map.type == MaterialMap::Type::Vector3 && map.vec.vectorIsColor == false){
            if(ImGui::DragFloat3(name.c_str(), &map.vec.vector[0])){
                toSave = true;
                isDirty = isDirtyUniformData = true;
            }
        }

        if(map.type == MaterialMap::Type::Vector4 && map.vec.vectorIsColor == false){
            if(ImGui::DragFloat4(name.c_str(), &map.vec.vector[0])){
                toSave = true;
                isDirty = isDirtyUniformData = true;
            }
        }

        if(map.type == MaterialMap::Type::Vector3 && map.vec.vectorIsColor == true){
            if(ImGui::ColorEdit3(name.c_str(), &map.vec.vector[0])){
                map.vec.vector = ToLinear(map.vec.vector);//TODO: Maybe check if this is realy need 
                toSave = true;
                isDirty = isDirtyUniformData = true;
            }
        }

        if(map.type == MaterialMap::Type::Vector4 && map.vec.vectorIsColor == true){
            if(ImGui::ColorEdit4(name.c_str(), &map.vec.vector[0]/*, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR*/)){
                map.vec.vector = ToLinear(map.vec.vector);//TODO: Maybe check if this is realy need
                toSave = true;
                isDirty = isDirtyUniformData = true;
            }
        }

        if(map.type == MaterialMap::Type::Texture){
            if(map.texture == nullptr) continue;
            const float widthSize = 60;
            float aspect = 1;

            if(map.texture != nullptr){
                Assert(map.texture->Height() != 0);
                aspect = map.texture->Width() / map.texture->Height();
            }

            ImGui::BeginGroup();
            ImVec2 imagePos = ImGui::GetCursorPos();
            //ImGui::Image((void*)(uint64_t)map.texture->RenderId(), ImVec2(widthSize, widthSize * aspect), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::Image(map.texture->RenderId(), ImVec2(widthSize, widthSize * aspect), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::SetCursorPos(imagePos);
            if(ImGui::SmallButton("X")){
                map.texture = Texture2D::LoadDefautlTexture2D();
                toSave = true;
                isDirty = isDirtyUniformData = true;
            }
            ImGui::EndGroup();
            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)){
                ImGui::_SelectionAsset(map.texture);
            }

            ImGui::AcceptFileMovePayload([&](std::filesystem::path* path){
                if(path->string().empty() == false && (path->extension() == ".png" || path->extension() == ".jpg")){
                    //LogInfo("Dragdrop Path: {}", path->generic_string());
                    //TODO: Maybe check if AssetManager::Get().LoadAsset<Texture2D> is valid
                    keepAlive = map.texture; // Avoid opengl crach on imgui becose text was deleted
                    map.texture = AssetManager::Get().LoadAsset<Texture2D>(path->generic_string());
                    toSave = true;
                    isDirty = isDirtyUniformData = true;
                }
            });

            ImGui::SameLine();
            ImGui::TextUnformatted(name.c_str()); //ImGui::Text(name.c_str());
        }
    }

    if(currentShader.drawTypes[0] != nullptr && currentShader.drawTypes[0]->pipeline.supportInstancing && ImGui::Checkbox("enableInstancing", &enableInstancing)){
        toSave = true;
        isDirty = isDirtyUniformData = true;
    }

    /*ImGui::Spacing();ImGui::Spacing();

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
    }*/

    //ImGui::EndDisabled();

    if(toSave && this->path.empty() == false && this->path != "Memory"){
        Save(this->path);
    }

    if(this->path.empty() == true || this->path == "Memory"){
        if(ImGui::Button("Save As")){
            std::string _path = Platform::SaveFile("*.material");
            if(_path.empty() == false){
                Save(_path);
            } 
        }
    }
}

void Material::Save(const std::string& path){
    //return;
    //Assert(false && "Not Implemented");

    LogInfo("Saving: {}", path);

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

bool Material::LoadFromPackage(const std::string& inPath, Package& package){
    void* data = nullptr;
    size_t size;
    if(package.ReadFileData(inPath.c_str(), data, size) == false){
        package.FreeFileData(data);
        return false;
    }

    path = inPath;

    try{
        MemoryInputStream mem((char*)data, size);
        cereal::JSONInputArchive archive{mem};
        archive(*this);
    } catch(...){
        package.FreeFileData(data);
        return false;
    }

    package.FreeFileData(data);
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
    //if(currentShader == nullptr) return;
    if(shader == nullptr) return;

    ///*

    //Remove Unused Maps
    for(auto i: shader->Properties() /*currentShader->Properties()*/){
        if(maps.count(i[1].c_str())) continue;
        maps.erase(i[1].c_str());
    }
    //maps.clear();
    properties.clear();

    // Add If Not Contains
    for(auto i: shader->Properties() /*currentShader->Properties()*/){
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
                Ref<Texture2D> tex = AssetManager::Get().LoadAsset<Texture2D>("Engine/Textures/White.jpg");
                Assert(tex != nullptr);
                Assert(tex->IsValid());
                SetTexture(i[1].c_str(), tex);
            } else if(i[2] == "Black"){
                Ref<Texture2D> tex = AssetManager::Get().LoadAsset<Texture2D>("Engine/Textures/Black.jpg");
                Assert(tex != nullptr);
                Assert(tex->IsValid());
                SetTexture(i[1].c_str(), tex );
            } else if(i[2] == "Normal"){
                Ref<Texture2D> tex = AssetManager::Get().LoadAsset<Texture2D>("Engine/Textures/Normal.jpg");
                Assert(tex != nullptr);
                Assert(tex->IsValid());
                SetTexture(i[1].c_str(), tex );
            } else {
                Ref<Texture2D> tex = Texture2D::LoadDefautlTexture2D();
                Assert(tex != nullptr);
                Assert(tex->IsValid());
                SetTexture(i[1].c_str(), tex);
            }
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Color4"){
            if(i.size() == 6){
                SetColor4(i[1].c_str(), Vector4(std::stof(i[2]), std::stof(i[3]), std::stof(i[4]), std::stof(i[5])));
            } else {
                SetColor4(i[1].c_str(), Vector4(1,1,1,1));
            }
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Color3"){
            SetColor3(i[1].c_str(), Vector3(1,1,1));
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Vector4"){
            if(i.size() == 6){
                SetVector4(i[1].c_str(), Vector4(std::stof(i[2]), std::stof(i[3]), std::stof(i[4]), std::stof(i[5])));
            } else {
                SetVector4(i[1].c_str(), Vector4(1,1,1,1));
            }
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Vector3"){
            SetVector3(i[1].c_str(), Vector3(1,1,1));
        }

        if(!maps.count(i[1].c_str()) && i[0] == "Vector2"){
            SetVector2(i[1].c_str(), Vector2(1,1));
        }
    }
    //*/

}

//void Material::ApplyUniformTo(Material& material, SubShader& shader, std::unordered_map<std::string, MaterialMap>& maps){
    //Shader::Bind(shader);
    /*
    for(auto& i: maps){
        MaterialMap& map = i.second;

        if(shader.ContainUniformName(i.first) == false) continue;

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
            Assert(i.second.texture != nullptr);
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
    }*/
//}

void Material::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Material>(
        "Material",
        "GetShader", &Material::GetShader,
        "SetShader", &Material::SetShader,
        "MaterialId", &Material::MaterialId,
        "IsBlend", &Material::IsBlend,
        "EnableInstancingValid", &Material::EnableInstancingValid,
        "EnableInstancing", &Material::EnableInstancing,
        "SupportInstancing", &Material::SupportInstancing,
        "SetInt", &Material::SetInt,
        //"SetFloat", &Material::SetFloat,
        "SetVector2", &Material::SetVector2,
        "SetVector3", &Material::SetVector3,
        //"SetVector4", &Material::SetVector4,
        //"SetMatrix4", &Material::SetMatrix4,
        //"SetTexture", &Material::SetTexture,
        "SetCubemap", &Material::SetCubemap,
        "SetGlobalInt", Material::SetGlobalInt,
        //"SetGlobalFloat", Material::SetGlobalFloat,
        "SetGlobalVector2", Material::SetGlobalVector2,
        "SetGlobalVector3", Material::SetGlobalVector3,
        //"SetGlobalVector4", Material::SetGlobalVector4,
        //"SetGlobalMatrix4", Material::SetGlobalMatrix4,
        //"SetGlobalTexture", Material::SetGlobalTexture,
        "SetGlobalCubemap", Material::SetGlobalCubemap,
        "DisableKeyword", &Material::DisableKeyword,
        "EnableKeyword", &Material::EnableKeyword,
        "CleanData", &Material::CleanData
    );
}

}