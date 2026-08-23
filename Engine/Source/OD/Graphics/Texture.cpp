#include "OD/pch.h"
#include "Texture.h"
#include "SubShader.h"
#include "OD/Core/Lua.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Package.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "OD/Serialization/SerializationFull.h"
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace OD{

extern GraphicsDevice* graphicsDevice;

Ref<Texture2D> Texture2D::CreateFromFile(const std::string& filePath, Texture2DSetting settings){
    Ref<Texture2D> tex = CreateRef<Texture2D>();
    tex->SetLoadSettings(settings);
    if(tex->LoadFromFile(filePath) == false){
        return nullptr;
    }

    return tex;
}

Ref<Texture2D> Texture2D::CreateFromFileMemory(void* data, size_t size, Texture2DSetting settings, const std::string& label){
    Ref<Texture2D> tex = CreateRef<Texture2D>();
    tex->SetLoadSettings(settings);
    if(tex->LoadFromFileMemory(data, size, label) == false){
        return nullptr;
    }
    return tex;
}

Ref<Texture2D> Texture2D::CreateFromRaw(void* data, int width, int height, TextureDataType dataType, Texture2DSetting settings, const std::string& label){
    Ref<Texture2D> tex = CreateRef<Texture2D>();
    tex->SetLoadSettings(settings);
    tex->settings = settings;
    if(graphicsDevice->Texture2DCreate(*tex, data, width, height, dataType) == false){
        graphicsDevice->Texture2DDestroy(*tex);
        return nullptr;
    }

    tex->path = "#" + label;
    return tex;
}

Ref<Texture2D> Texture2D::CreateFromPackage(const char* path, Package& package, Texture2DSetting settings){
    Ref<Texture2D> out = CreateRef<Texture2D>();
    out->SetLoadSettings(settings);
    if(out->LoadFromPackage(path, package)){
        return out;
    }
    return out;

    void* data = nullptr;
    size_t size;
    if(package.ReadFileData(path, data, size) == false){
        package.FreeFileData(data);
        return nullptr;
    }
    
    Ref<Texture2D> tex = CreateRef<Texture2D>();
    
    std::string inpath(path);
    if(inpath.empty() == false && inpath[0] != '#') LoadArchive(package, inpath + ".meta", settings, "settings");

    tex->SetLoadSettings(settings);
    if(tex->LoadFromFileMemory(data, size) == false){ //if(graphicsDevice->Texture2DCreate(*tex, data, size) == false){
        package.FreeFileData(data);
        return nullptr;
    }

    package.FreeFileData(data);
    return tex;
}

void Texture2D::SetLoadSettings(Texture2DSetting inloadSettings){ 
    loadSettings = inloadSettings; 
}

bool Texture2D::LoadFromFileMemory(void* indata, size_t insize, const std::string& label){
    settings = loadSettings;
    //rif(inpath.empty() == false && inpath[0] != '#') LoadOrCreateArchive(path + ".meta", settings, "settings");

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* data = stbi_load_from_memory((const stbi_uc*)indata, insize, &width, &height, &nrChannels, 0);

    if(!data){
        LogError("Cannot load file image {}\nSTB Reason: {}\n", path, stbi_failure_reason());
        stbi_image_free(data);
        return false;
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(alpha){
        settings.textureFormat = TextureFormat::RGBA;
        //Assert(size % (sizeof(unsigned char)*4) == 0);
    } else {
        settings.textureFormat = TextureFormat::RGB;
        //Assert(size % (sizeof(unsigned char)*3) == 0);
    }

    if(graphicsDevice->Texture2DCreate(*this, data, width, height, TextureDataType::UnsignedByte) == false){
        graphicsDevice->Texture2DDestroy(*this);
        stbi_image_free(data);
        return false;
    }

    path = "#"+label;
    stbi_image_free(data);
    return true;
}

bool Texture2D::LoadFromFile(const std::string& inpath){
    /*settings = loadSettings;

    if(inpath.empty() == false && inpath[0] != '#') LoadOrCreateArchive(inpath + ".meta", settings, "settings");

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* data = stbi_load(inpath.c_str(), &width, &height, &nrChannels, 0);

    if(!data){
        LogError("Cannot load file image {}\nSTB Reason: {}\n", inpath, stbi_failure_reason());
        stbi_image_free(data);
        return false;
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(alpha){
        settings.textureFormat = TextureFormat::RGBA;
    } else {
        settings.textureFormat = TextureFormat::RGB;
    }

    if(graphicsDevice->Texture2DCreate(*this, data, width, height, TextureDataType::UnsignedByte) == false){
        graphicsDevice->Texture2DDestroy(*this);
        stbi_image_free(data);
        return false;
    }

    path = inpath;
    stbi_image_free(data);
    return true;*/

    
    namespace fs = std::filesystem;
    
    if(fs::exists(inpath) == false) return false;
    if(inpath.empty()) return false;

    fs::path p(inpath);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    //Lambda for user-friendly image formats
    auto LoadFromImageFile = [&](const std::string& path){
        settings = loadSettings;

        // Only image formats use .meta
        if(path[0] != '#') LoadOrCreateArchive(path + ".meta", settings, "settings");

        stbi_set_flip_vertically_on_load(1);

        int width = 0;
        int height = 0;
        int nrChannels = 0;

        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

        if(!data){
            LogError("Cannot load file image {}\nSTB Reason: {}\n", path, stbi_failure_reason());
            return false;
        }

        //settings.textureFormat = (nrChannels > 3) ? TextureFormat::RGBA : TextureFormat::RGB;
        if(nrChannels == 4){
            settings.textureFormat = TextureFormat::RGBA;
        } else if(nrChannels == 3){
            settings.textureFormat = TextureFormat::RGB;
        } else if(nrChannels == 1){
            settings.textureFormat = TextureFormat::RED8;
        } else {
            Assert(false && "Not supported yet!!!");
        }

        bool success = graphicsDevice->Texture2DCreate(
            *this,
            data,
            width,
            height,
            TextureDataType::UnsignedByte
        );

        stbi_image_free(data);

        if(!success){
            graphicsDevice->Texture2DDestroy(*this);
            return false;
        }

        this->path = path;
        return true;
    };

    auto LoadFromBinaryFile = [&](const std::string& path){
        std::ifstream is(path, std::ios::binary);
        if(!is.is_open()) return false;

        cereal::PortableBinaryInputArchive archive{is};

        int width = 0;
        int height = 0;
        int nrChannels = 0;
        size_t size = 0;

        archive(settings);
        archive(width);
        archive(height);
        archive(nrChannels);
        archive(size);

        std::vector<uint8_t> data(size);
        archive(cereal::binary_data(data.data(), size));

        //settings.textureFormat = (nrChannels > 3) ? TextureFormat::RGBA : TextureFormat::RGB;
        if(nrChannels == 4){
            settings.textureFormat = TextureFormat::RGBA;
        } else if(nrChannels == 3){
            settings.textureFormat = TextureFormat::RGB;
        } else if(nrChannels == 1){
            settings.textureFormat = TextureFormat::RED8;
        } else {
            Assert(false && "Not supported yet!!!");
        }

        bool success = graphicsDevice->Texture2DCreate(
            *this,
            data.data(),
            width,
            height,
            TextureDataType::UnsignedByte
        );

        if(!success){
            graphicsDevice->Texture2DDestroy(*this);
            return false;
        }

        this->path = path;
        return true;
    };

    if(ext == ".texturebin"){
        return LoadFromBinaryFile(inpath);
    }

    return LoadFromImageFile(inpath);
}

bool Texture2D::LoadFromPackage(const std::string& path, Package& package){
    void* data = nullptr;
    size_t size;
    if(package.ReadFileData(path.c_str(), data, size) == false){
        package.FreeFileData(data);
        return false;
    }

    namespace fs = std::filesystem;
    fs::path p(path);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto LoadFromBinaryFile = [&](){
        MemoryInputStream mem((char*)data, size);
        cereal::PortableBinaryInputArchive archive{mem};

        int width = 0;
        int height = 0;
        int nrChannels = 0;
        size_t size = 0;

        archive(settings);
        archive(width);
        archive(height);
        archive(nrChannels);
        archive(size);

        std::vector<uint8_t> data(size);
        archive(cereal::binary_data(data.data(), size));

        //settings.textureFormat = (nrChannels > 3) ? TextureFormat::RGBA : TextureFormat::RGB;
        if(nrChannels == 4){
            settings.textureFormat = TextureFormat::RGBA;
        } else if(nrChannels == 3){
            settings.textureFormat = TextureFormat::RGB;
        } else if(nrChannels == 1){
            settings.textureFormat = TextureFormat::RED8;
        } else {
            Assert(false && "Not supported yet!!!");
        }

        bool success = graphicsDevice->Texture2DCreate(
            *this,
            data.data(),
            width,
            height,
            TextureDataType::UnsignedByte
        );

        if(!success){
            graphicsDevice->Texture2DDestroy(*this);
            return false;
        }

        this->path = path;
        return true;
    };

    if(ext == ".texturebin"){
        return LoadFromBinaryFile();
    }

    std::string inpath(path);
    if(inpath.empty() == false && inpath[0] != '#'){
        LoadArchive(package, inpath + ".meta", loadSettings, "settings");
    }

    //SetLoadSettings(settings);
    if(LoadFromFileMemory(data, size) == false){
        package.FreeFileData(data);
        return false;
    }

    package.FreeFileData(data);
    return true;
}

std::vector<std::string> Texture2D::GetFileAssociations(){ 
    return std::vector<std::string>{
        ".jpg",
        ".png",
        ".texturebin"
    }; 
}

Ref<Texture2D> Texture2D::LoadDefautlTexture2D(){
    return ResourceManager::Get().LoadByPath<Texture2D>("Engine/Textures/White.jpg");
}

Ref<Texture2D> Texture2D::CreateBrdfLUTTexture2D(){
    return graphicsDevice->Texture2DCreateBrdfLUTTexture2D();
    
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

Texture2D::Texture2D(){
    //LogInfo("OnCreation");
}

Texture2D::~Texture2D(){
    //LogInfo("OnDestroy: {}", path);
    Assert(graphicsDevice != nullptr);
    graphicsDevice->Texture2DDestroy(*this);
}

bool Texture2D::IsValid(){
    Assert(graphicsDevice != nullptr);
    return graphicsDevice->Texture2DIsValid(*this);
}

unsigned int Texture2D::Width(){ 
    return width; 
}

unsigned int Texture2D::Height(){ 
    return height; 
}

void* Texture2D::RenderId(){
    return graphicsDevice->Texture2DRenderId(*this);
}

void Texture2D::OnGui(){
    bool save = false;

    //ImGui::Text("--------Texture2D--------");
    //ImGui::Text("Path: %s", path().c_str());

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

    float aspect = width / height;
    ImGui::Image(RenderId(), ImVec2(viewportPanelSize.x, viewportPanelSize.x * aspect), ImVec2(0, 1), ImVec2(1, 0));

    ImGui::Spacing();

    const char* optionsString[] = {"Nearest", "Linear"};
    const char* curOptionString = optionsString[(int)settings.filter];

    auto formatName = std::string(magic_enum::enum_name(settings.textureFormat));
    ImGui::Text("Format: %s", formatName.c_str());

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
        Reload();
        //Save();
    }
}

void Texture2D::Reload(){
    if(path == "Memory"){
        LogError("Can Not Reload Texture2d From Memory");
        return;
    }

    SaveArchive(path + ".meta", settings, "settings");
    LoadFromFile(path);
}

bool Texture2D::Save(const std::string& outPath, SaveType type){
    if(type == Resource::SaveType::SettingOnly){
        std::ofstream os(path + ".meta");
        if(os.is_open() == false) return false;
        
        cereal::JSONOutputArchive archive{os};
        archive(CEREAL_NVP(settings));
    }

    if(type == Resource::SaveType::FinalBinary){
        std::ofstream os(outPath, std::ios::binary);
        if(os.is_open() == false) return false;

        stbi_set_flip_vertically_on_load(1);
        int width = 0;
        int height = 0;
        int nrChannels = 0;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
        if(!data){
            LogError("Cannot load file image {}\nSTB Reason: {}\n", path, stbi_failure_reason());
            return false;
        }

        size_t size =
        static_cast<size_t>(width) *
        static_cast<size_t>(height) *
        static_cast<size_t>(nrChannels);

        cereal::PortableBinaryOutputArchive archive{os};
        archive(settings);
        archive(width);
        archive(height);
        archive(nrChannels);
        archive(size);
        archive(cereal::binary_data(data, size));

        stbi_image_free(data);
    }

    return false;

    /*if(path.empty() == false && path != "Memory"){
        std::ofstream os(path + ".meta");
        cereal::JSONOutputArchive archive{os};
        archive(CEREAL_NVP(settings));
    }*/

    //Destroy(*this);
    //Reload();
}

bool Texture2D::GetPixelData(std::vector<uint8_t>& outData){
    Assert(graphicsDevice != nullptr);
    return graphicsDevice->Texture2DGetPixelData(*this, outData);
}

void Texture2D::SaveTo(cereal::BinaryOutputArchive& ar){
    Assert(settings.textureFormat == TextureFormat::RGBA || settings.textureFormat == TextureFormat::RGB);
    ar(settings);

    int w = Width();
    int h = Height();
    ar(w);
    ar(h);

    /*std::vector<uint8_t> pixelData;
    GetPixelData(pixelData);
    ar(pixelData);*/
    
    std::vector<uint8_t> pixelData;
    GetPixelData(pixelData);

    std::vector<uint8_t> pngData;

    if(settings.textureFormat == TextureFormat::RGBA){
        int a = stbi_write_png_to_func(
            [](void* ctx, void* data, int size) {
                auto* out = static_cast<std::vector<uint8_t>*>(ctx);
                uint8_t* bytes = (uint8_t*)data;
                out->insert(out->end(), bytes, bytes + size);
            },
            &pngData,
            w, h, 4,
            pixelData.data(),
            0
        );
        Assert(a > 0);
    }

    if(settings.textureFormat == TextureFormat::RGB){
        int a = stbi_write_jpg_to_func(
            [](void* ctx, void* data, int size) {
                auto* out = static_cast<std::vector<uint8_t>*>(ctx);
                uint8_t* bytes = (uint8_t*)data;
                out->insert(out->end(), bytes, bytes + size);
            },
            &pngData,
            w, h, 3,
            pixelData.data(),
            0
        );
        Assert(a > 0);
    }

    int pngSize = pngData.size();
    ar(pngSize);
    ar(cereal::binary_data(pngData.data(), pngSize));
}

void Texture2D::LoadFrom(cereal::BinaryInputArchive& ar){
    ar(settings);
    Assert(settings.textureFormat == TextureFormat::RGBA || settings.textureFormat == TextureFormat::RGB);

    int w, h;
    ar(w);
    ar(h);

    /*std::vector<uint8_t> pixelData;
    ar(pixelData);
    if(graphicsDevice->Texture2DCreate(*this, pixelData.data(), 0, w, h, TextureDataType::UnsignedByte) == false){
        Assert(false);
        graphicsDevice->Texture2DDestroy(*this);
    }*/

    int pngSize;
    ar(pngSize);

    std::vector<uint8_t> pngData(pngSize);
    ar(cereal::binary_data(pngData.data(), pngSize));

    int ow, oh, nch;
    unsigned char* decoded = stbi_load_from_memory(
        pngData.data(), pngSize,
        &ow, &oh, &nch, settings.textureFormat == TextureFormat::RGBA ? 4 : 3
    );
    if(settings.textureFormat == TextureFormat::RGBA){
        Assert(nch == 4);
    } else {
        Assert(nch == 3);
    }

    if(!decoded){
        Assert(false);
        //throw std::runtime_error("Failed to decode PNG in Model::LoadTo");
    }

    if(graphicsDevice->Texture2DCreate(*this, decoded, w, h, TextureDataType::UnsignedByte) == false){
        Assert(false);
        graphicsDevice->Texture2DDestroy(*this);
    }

    stbi_image_free(decoded);
}

void Texture2D::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Texture2DSetting>(
        sol::base_classes, sol::bases<Resource>(),
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
        sol::call_constructor, Texture2D::CreateFromFile,
        "New", Texture2D::CreateFromFile,
        "CreateFromFile", Texture2D::CreateFromFile,
        "CreateFromFileMemory", Texture2D::CreateFromFileMemory,
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