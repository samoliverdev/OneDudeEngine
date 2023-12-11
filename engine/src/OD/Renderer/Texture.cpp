#include "Texture.h"
#include "OD/Core/ImGui.h"
#include <stb/stb_image.h>
#include "OD/Serialization/Serialization.h"

namespace OD{

const int TextureFilterLookup[] = {
    GL_NEAREST,
    GL_LINEAR
};

const int TextureFilterLookupMipmap[] = {
    GL_NEAREST_MIPMAP_NEAREST,
    GL_LINEAR_MIPMAP_LINEAR
};

void LoadSettings(const char* filePath, Texture2DSetting& settings);

bool Texture2D::CreateFromFile(Texture2D& tex, const std::string& filePath, Texture2DSetting settings){
    tex.path = std::string(filePath);
    tex.settings = settings;

    LoadSettings(tex.path.c_str(), tex.settings);

    tex.internalFormat = GL_RGB;
    tex.imageFormat = GL_RGB;
    tex.wrapS = GL_REPEAT;
    tex.wrapT = GL_REPEAT;
    tex.filterMin = TextureFilterLookup[(int)settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(settings.mipmap){
        tex.filterMin = TextureFilterLookupMipmap[(int)settings.filter];
    }
    tex.filterMax = TextureFilterLookup[(int)settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    tex.mipmap = settings.mipmap;

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* data = stbi_load(tex.path.c_str(), &width, &height, &nrChannels, 0);

    //LogInfo("Chennels %d", nrChannels);

    if(!data){
        LogError("Cannot load file image %s\nSTB Reason: %s\n", tex.path.c_str(), stbi_failure_reason());
        return false;
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(alpha){
        tex.internalFormat = GL_RGBA;
        tex.imageFormat = GL_RGBA;
    }
    
    tex.texture2DGenerate(width, height, data);
    stbi_image_free(data);

    return true;
}

bool Texture2D::CreateFromFileMemory(Texture2D& tex, void* data, size_t size, Texture2DSetting settings){
    tex.path = "Memory";
    tex.settings = settings;

    tex.internalFormat = GL_RGB;
    tex.imageFormat = GL_RGB;
    tex.wrapS = GL_REPEAT;
    tex.wrapT = GL_REPEAT;
    tex.filterMin = TextureFilterLookup[(int)settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(settings.mipmap){
        tex.filterMin = TextureFilterLookupMipmap[(int)settings.filter];
    }
    tex.filterMax = TextureFilterLookup[(int)settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    tex.mipmap = settings.mipmap;

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* _data = stbi_load_from_memory((const stbi_uc*)data, size, &width, &height, &nrChannels, 0);

    //LogInfo("Chennels %d", nrChannels);

    if(!_data){
        LogError("Cannot load file image %s\nSTB Reason: %s\n", tex.path.c_str(), stbi_failure_reason());
        return false;
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(alpha){
        tex.internalFormat = GL_RGBA;
        tex.imageFormat = GL_RGBA;
    }
    
    tex.texture2DGenerate(width, height, _data);
    stbi_image_free(_data);

    return true;
}

void Texture2D::Destroy(Texture2D& tex){
    if(tex.id != 0) glDeleteTextures(1, &tex.id);
    tex.id = 0;
    glCheckError();
}

void Texture2D::Bind(Texture2D& tex, int index){
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glCheckError();
}

Texture2D::~Texture2D(){
    Texture2D::Destroy(*this);
}

bool Texture2D::IsValid(){
    return id != 0;
}

void Texture2D::texture2DGenerate(unsigned int inWidth, unsigned int inHeight, unsigned char* data){
    width = inWidth;
    height = inHeight;
    
    glGenTextures(1, &id);
    glCheckError();

    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, imageFormat, GL_UNSIGNED_BYTE, data);
    glCheckError();
    
    if(mipmap == true) glGenerateMipmap(GL_TEXTURE_2D);
    glCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMax);
    glCheckError();

    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
}

void Texture2D::OnGui(){
    bool save = false;

    //ImGui::Text("--------Texture2D--------");
    //ImGui::Text("Path: %s", path().c_str());

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

    float aspect = width / height;
    ImGui::Image((void*)(uint64_t)id, ImVec2(viewportPanelSize.x, viewportPanelSize.x * aspect), ImVec2(0, 0), ImVec2(1, -1));

    ImGui::Spacing();

    const char* optionsString[] = {"Nearest", "Linear"};
    const char* curOptionString = optionsString[(int)settings.filter];

    if(ImGui::BeginCombo("filter", curOptionString)){
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
    }

    if(ImGui::Checkbox("mipmap", &settings.mipmap)){
        save = true;
    }

    ImGui::Spacing();

    ImGui::Text("Path: %s", Path().c_str());
    ImGui::Text("Width: %d Height: %d", width, height);

    if(save){
        SaveSettings();
    }
}

void Texture2D::SaveSettings(){

    if(path.empty() == false && path != "Memory"){
        std::ofstream os(path + ".meta");
        cereal::JSONOutputArchive archive{os};
        archive(CEREAL_NVP(settings));
    }

    Destroy(*this);
    CreateFromFile(*this, path.c_str(), settings);
}

void LoadSettings(const char* filePath, Texture2DSetting& settings){
    std::ifstream stream(std::string(filePath) + ".meta");
    if(stream.fail()) return;

    LogInfo("tttdss");

    cereal::JSONInputArchive archive{stream};
    archive(CEREAL_NVP(settings));
}

}