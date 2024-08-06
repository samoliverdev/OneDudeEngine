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

const int TextureWrappingLookupMipmap[] = {
    GL_REPEAT,
    GL_MIRRORED_REPEAT,
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_BORDER
};

const int TextureFormatLookupMipmap[] = {
    GL_NONE,
    GL_RGB,
    GL_RGBA,
    //GL_RGB,
    //GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,
};

const int TextureInternalFormatLookupMipmap[] = {
    GL_NONE,
    GL_RGB,
    GL_RGBA,
    //GL_SRGB,
    //GL_SRGB_ALPHA,

    GL_R8,
    GL_RGB8,
    GL_RGBA8,

    GL_R16,
    GL_RGB16,
    GL_RGBA16,

    GL_R16F,
    GL_RGB16F,
    GL_RGBA16F,

    GL_R32F,
    GL_RGB32F,
    GL_RGBA32F,
};

const int TextureDataTypeFormatLookupMipmap[] = {
    GL_UNSIGNED_BYTE,
    GL_UNSIGNED_INT,
    GL_INT,
    GL_FLOAT
};

Texture2D::Texture2D(Texture2DSetting inSettings){
    settings = inSettings;
}

Texture2D::Texture2D(const std::string& filePath, Texture2DSetting settings){
    if(Create(path, settings) == false){
        Destroy(*this);
    }
}

Texture2D::Texture2D(void* data, size_t size, Texture2DSetting settings){
    if(Create(data, size, settings) == false){
        Destroy(*this);
    }
} 

Texture2D::Texture2D(void* data, size_t size, int width, int height,  TextureDataType dataType, Texture2DSetting settings){
    if(Create(data, size, width, height, dataType, settings) == false){
        Destroy(*this);
    }
}

bool Texture2D::LoadFromFile(const std::string& path){
    if(Create(path, settings) == false){
        Destroy(*this);
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

void LoadSettings(const char* filePath, Texture2DSetting& settings);

Ref<Texture2D> Texture2D::CreateFromFile(const std::string& filePath, Texture2DSetting settings){
    Ref<Texture2D> tex = CreateRef<Texture2D>();
    if(tex->Create(filePath, settings) == false){
        Destroy(*tex);
        return nullptr;
    }

    return tex;
}

Ref<Texture2D> Texture2D::CreateFromFileMemory(void* data, size_t size, Texture2DSetting settings){
    Ref<Texture2D> tex = CreateRef<Texture2D>();
    if(tex->Create(data, size, settings) == false){
        Destroy(*tex);
        return nullptr;
    }

    return tex;
}

Ref<Texture2D> Texture2D::LoadDefautlTexture2D(){
    return AssetManager::Get().LoadAsset<Texture2D>("res/Engine/Textures/White.jpg");
}

void Texture2D::Destroy(Texture2D& tex){
    if(tex.IsValid() == false) return;

    glDeleteTextures(1, &tex.id);
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

bool Texture2D::Create(const std::string path, Texture2DSetting settings){
    Destroy(*this);

    this->path = std::string(path);
    this->settings = settings;
    LoadSettings(this->path.c_str(), this->settings);
    this->wrapS = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    this->wrapT = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    this->filterMin = TextureFilterLookup[(int)settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(settings.mipmap){
        this->filterMin = TextureFilterLookupMipmap[(int)settings.filter];
    }
    this->filterMax = TextureFilterLookup[(int)settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    this->mipmap = settings.mipmap;

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* data = stbi_load(this->path.c_str(), &width, &height, &nrChannels, 0);

    //LogInfo("Chennels %d", nrChannels);

    if(!data){
        LogError("Cannot load file image %s\nSTB Reason: %s\n", this->path.c_str(), stbi_failure_reason());
        return false;
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(settings.textureFormat == TextureFormat::Auto){
        if(alpha){
            this->internalFormat = GL_RGBA; //GL_SRGB_ALPHA; //GL_RGBA;
            this->imageFormat = GL_RGBA;
        } else {
            this->internalFormat = GL_RGB; //GL_SRGB_ALPHA; //GL_RGBA;
            this->imageFormat = GL_RGB;
        }
    } else {
        this->internalFormat = TextureInternalFormatLookupMipmap[(int)settings.textureFormat];
        this->imageFormat = TextureFormatLookupMipmap[(int)settings.textureFormat];
    }
    
    this->texture2DGenerate(width, height, TextureDataType::UnsignedByte, data);
    stbi_image_free(data);

    return true;
}

bool Texture2D::Create(void* data, size_t size, Texture2DSetting settings){
    Destroy(*this);

    this->path = "Memory";
    this->settings = settings;
    this->wrapS = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    this->wrapT = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    this->filterMin = TextureFilterLookup[(int)settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(settings.mipmap){
        this->filterMin = TextureFilterLookupMipmap[(int)settings.filter];
    }
    this->filterMax = TextureFilterLookup[(int)settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    this->mipmap = settings.mipmap;

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* _data = stbi_load_from_memory((const stbi_uc*)data, size, &width, &height, &nrChannels, 0);

    //LogInfo("Chennels %d", nrChannels);

    if(!_data){
        LogError("Cannot load file image %s\nSTB Reason: %s\n", this->path.c_str(), stbi_failure_reason());
        return false;
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(settings.textureFormat == TextureFormat::Auto){
        if(alpha){
            this->internalFormat = GL_RGBA; //GL_SRGB_ALPHA; //GL_RGBA;
            this->imageFormat = GL_RGBA;
        } else {
            this->internalFormat = GL_RGB; //GL_SRGB_ALPHA; //GL_RGBA;
            this->imageFormat = GL_RGB;
        }
    } else {
        this->internalFormat = TextureInternalFormatLookupMipmap[(int)settings.textureFormat];
        this->imageFormat = TextureFormatLookupMipmap[(int)settings.textureFormat];
    }

    
    this->texture2DGenerate(width, height, TextureDataType::UnsignedByte, _data);
    stbi_image_free(_data);

    return true;
}

bool Texture2D::Create(void* data, size_t size, int width, int height, TextureDataType dataType, Texture2DSetting settings){
    Destroy(*this);

    this->path = "Memory";
    this->settings = settings;
    this->wrapS = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    this->wrapT = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    this->filterMin = TextureFilterLookup[(int)settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(settings.mipmap){
        this->filterMin = TextureFilterLookupMipmap[(int)settings.filter];
    }
    this->filterMax = TextureFilterLookup[(int)settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    this->mipmap = settings.mipmap;

    Assert(settings.textureFormat != TextureFormat::Auto);

    this->internalFormat = TextureInternalFormatLookupMipmap[(int)settings.textureFormat];
    this->imageFormat = TextureFormatLookupMipmap[(int)settings.textureFormat];
    this->texture2DGenerate(width, height, dataType, data);

    return true;
}

void Texture2D::texture2DGenerate(unsigned int inWidth, unsigned int inHeight, TextureDataType dataType, void* data){
    width = inWidth;
    height = inHeight;
    
    glGenTextures(1, &id);
    glCheckError();

    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, imageFormat, TextureDataTypeFormatLookupMipmap[(int)dataType], data);
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

void LoadSettings(const char* filePath, Texture2DSetting& settings){
    std::ifstream stream(std::string(filePath) + ".meta");
    if(stream.fail()) return;

    cereal::JSONInputArchive archive{stream};
    archive(CEREAL_NVP(settings));
}

Texture2DArray::Texture2DArray(const std::vector<std::string>& filePaths){
    stbi_set_flip_vertically_on_load(1);

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);
    glCheckError();

    std::vector<unsigned char*> datas;
    
    int width, height, nrComponents;
    unsigned int internalFormat = GL_RGB8;
    unsigned int imageFormat = GL_RGB;
    unsigned int mipLevelCount = 8;
    for(auto& i: filePaths){
        unsigned char* data = stbi_load(i.c_str(), &width, &height, &nrComponents, 0);   // Load the first Image of size 512   X   512  (RGB))

        if(!data){
            LogError("Cannot load file image %s\nSTB Reason: %s\n", this->path.c_str(), stbi_failure_reason());
        }

        if(nrComponents > 3){
            internalFormat = GL_RGBA8; //GL_SRGB_ALPHA; //GL_RGBA;
            imageFormat = GL_RGBA;
            //mipLevelCount = 1;
        }

        datas.push_back(data);
    }
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, internalFormat, width, height, filePaths.size());
    glCheckError();
   
    int _i = 0;
    for(auto i: datas){
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, _i, width, height, 1, imageFormat, GL_UNSIGNED_BYTE, i);
        glCheckError();
        _i += 1;
    }

    if(mipLevelCount > 1) glGenerateMipmap(GL_TEXTURE_2D_ARRAY); 
    glCheckError();

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glCheckError();

    for(auto i: datas){
        stbi_image_free(i);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glCheckError();
}

Texture2DArray::~Texture2DArray(){
    Texture2DArray::Destroy(*this);
}

void Texture2DArray::Destroy(Texture2DArray& tex){
    if(tex.IsValid() == false) return;

    glDeleteTextures(1, &tex.id);
    tex.id = 0;
    glCheckError();
}

void Texture2DArray::Bind(Texture2DArray& tex, int index){
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.id);
    glCheckError();
}

bool Texture2DArray::IsValid(){
    return id != 0;
}

void Texture2DArray::OnGui(){

}

}