#include "Texture.h"
#include "SubShader.h"
#include "OD/Core/Lua.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Package.h"
#include "Graphics.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/SerializationFull.h"
#include <fstream>
#include <stb/stb_image.h>

namespace OD{

extern GraphicsDevice* graphicsDevice;

Ref<Texture2D> Texture2D::CreateFromFile(const std::string& filePath, Texture2DSetting settings){
    Ref<Texture2D> tex = CreateRef<Texture2D>();
    if(graphicsDevice->Texture2DCreate(*tex, filePath, settings) == false){
        graphicsDevice->Texture2DDestroy(*tex);
        return nullptr;
    }

    return tex;
}

Ref<Texture2D> Texture2D::CreateFromMemory(void* data, size_t size, Texture2DSetting settings){
    Ref<Texture2D> tex = CreateRef<Texture2D>();
    if(graphicsDevice->Texture2DCreate(*tex, data, size, settings) == false){
        graphicsDevice->Texture2DDestroy(*tex);
        return nullptr;
    }

    return tex;
}

Ref<Texture2D> Texture2D::CreateFromRaw(void* data, size_t size, int width, int height, TextureDataType dataType, Texture2DSetting settings){
    Ref<Texture2D> tex = CreateRef<Texture2D>();
    if(graphicsDevice->Texture2DCreate(*tex, data, size, width, height, dataType, settings) == false){
        graphicsDevice->Texture2DDestroy(*tex);
        return nullptr;
    }

    return tex;
}

Ref<Texture2D> Texture2D::CreateFromPackage(const char* path, Package& package, Texture2DSetting settings){
    void* data = nullptr;
    size_t size;
    if(package.ReadFile(path, data, size) == false) return nullptr;

    Ref<Texture2D> tex = CreateRef<Texture2D>();
    if(graphicsDevice->Texture2DCreate(*tex, data, size, settings) == false){
        graphicsDevice->Texture2DDestroy(*tex);
        return nullptr;
    }

    return tex;
}

bool Texture2D::LoadFromFile(const std::string& path){
    if(graphicsDevice->Texture2DCreate(*this, path, settings) == false){
        graphicsDevice->Texture2DDestroy(*this);
        return false;
    }

    return true;
}

std::vector<std::string> Texture2D::GetFileAssociations(){ 
    return std::vector<std::string>{
        ".jpg",
        ".png"
    }; 
}

Ref<Texture2D> Texture2D::LoadDefautlTexture2D(){
    return AssetManager::Get().LoadAsset<Texture2D>("Engine/Textures/White.jpg");
}

Ref<Texture2D> Texture2D::CreateBrdfLUTTexture2D(){
    Assert(false && "Not Work for now");
    return nullptr;
    
    /*Assert(Graphics::HasBegin() == false);

    // pbr: setup framebuffer
    // ----------------------
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glCheckError();
    
    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    unsigned int brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);
    glCheckError();

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);
    glCheckError();

    
    Ref<SubShader> brdfShader = AssetManager::Get().LoadAsset<SubShader>("Engine/Shaders/brdf.glsl");// Shader::CreateFromFile("Engine/Shaders/brdf.glsl");
    SubShader::Bind(*brdfShader);

    glViewport(0, 0, 512, 512);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCheckError();

    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;
    renderQuad(quadVAO, quadVBO);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();

    Ref<Texture2D> out = CreateRef<Texture2D>();
    out->id = brdfLUTTexture;
    out->width = 512;
    out->height = 512;
    return out;
    */
}

Texture2D::~Texture2D(){
    graphicsDevice->Texture2DDestroy(*this);
}

bool Texture2D::IsValid(){
    return graphicsDevice->Texture2DIsValid(*this);
}

void Texture2D::OnGui(){
    bool save = false;

    //ImGui::Text("--------Texture2D--------");
    //ImGui::Text("Path: %s", path().c_str());

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

    float aspect = width / height;
    //ImGui::Image((void*)(uint64_t)id, ImVec2(viewportPanelSize.x, viewportPanelSize.x * aspect), ImVec2(0, 1), ImVec2(1, 0));

    ImGui::Spacing();

    const char* optionsString[] = {"Nearest", "Linear"};
    const char* curOptionString = optionsString[(int)settings.filter];

    if(ImGui::DrawEnumCombo<TextureFilter>("filter", &settings.filter)){
        save = true;
    }

    if(ImGui::DrawEnumCombo<TextureWrapping>("wrap", &settings.wrap)){
        save = true;
    }

    /*if(ImGui::BeginCombo("filter", curOptionString)){
        for(int i = 0; i < 2; i++){
            bool isSelected = curOptionString == optionsString[i];
            if(ImGui::Selectable(optionsString[i], isSelected)){
                curOptionString = optionsString[i];
                settings.filter = (TextureFilter)i;
                save = true;
            }
            if(isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }*/

    if(ImGui::Checkbox("mipmap", &settings.mipmap)){
        save = true;
    }

    //if(ImGui::DrawEnumCombo<TextureFormat>("textureFormat", &settings.textureFormat)){
    //    save = true;
    //}

    ImGui::Spacing();

    ImGui::Text("Path: %s", Path().c_str());
    ImGui::Text("Width: %d Height: %d", width, height);

    if(save){
        Save();
    }
}

void Texture2D::Reload(){
    if(path == "Memory"){
        LogError("Can Not Reload Texture2d From Memory");
        return;
    }

    CreateFromFile(path, settings);
}

void Texture2D::Save(){

    if(path.empty() == false && path != "Memory"){
        std::ofstream os(path + ".meta");
        cereal::JSONOutputArchive archive{os};
        archive(CEREAL_NVP(settings));
    }

    //Destroy(*this);
    Reload();
}

void Texture2D::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Texture2DSetting>(
        "Texture2DSetting",
        sol::call_constructor,
        sol::constructors<void()>(),
        "filter", &Texture2DSetting::filter,
        "wrap", &Texture2DSetting::filter,
        "mipmap", &Texture2DSetting::filter,
        "textureFormat", &Texture2DSetting::filter
    );

    lua.new_usertype<Texture2D>(
        "Texture2D",
        "New", Texture2D::CreateFromFile,
        "CreateFromFile", Texture2D::CreateFromFile,
        "CreateFromMemory", Texture2D::CreateFromMemory,
        "CreateFromPackage", Texture2D::CreateFromPackage,
        "LoadDefautlTexture2D", Texture2D::LoadDefautlTexture2D,
        "CreateBrdfLUTTexture2D", Texture2D::CreateBrdfLUTTexture2D,
        //"Destroy", Texture2D::Destroy,
        //"Bind", Texture2D::Bind,
        "IsValid", &Texture2D::IsValid,
        "Width", &Texture2D::Width,
        "Height", &Texture2D::Height,
        "RenderId", &Texture2D::RenderId
    );
}

Texture2DArray::Texture2DArray(const std::vector<std::string>& filePaths){
    graphicsDevice->Texture2DArrayCreate(*this, filePaths);
}

Texture2DArray::~Texture2DArray(){
    graphicsDevice->Texture2DArrayDestroy(*this);
}

void Texture2DArray::OnGui(){

}

}