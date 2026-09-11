#pragma once
#include "OD/Core/Resource.h"
#include "OD/Platform/OpenGL/GL.h"
#include "OD/Platform/WebGPU/WebGPU.h"
#include "OD/Gfx/Gfx.h"

namespace cereal{
    class BinaryOutputArchive;
    class BinaryInputArchive;
}

namespace sol{ class state; }

namespace OD {

class Package;

enum class OD_API_IMPORT TextureFilter {
    Nearest,
    Linear
};

enum class OD_API_IMPORT TextureWrapping{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class OD_API_IMPORT TextureDataType{
    UnsignedByte,
    UnsignedInt,
    Int,
    Float
};

enum class OD_API_IMPORT TextureFormat{
    None,
    RGB,
    RGBA,
    //SRGB,
    //SRGBA,

    RED8,
    RGB8,
    RGBA8,

    RED16,
    RGB16,
    RGBA16,

    RED16F,
    RGB16F,
    RGBA16F,

    RED32F,
    RGB32F,
    RGBA32F,
};

class Graphics;

struct OD_API Texture2DSetting{
    TextureFilter filter = TextureFilter::Linear;
    TextureWrapping wrap = TextureWrapping::Repeat;
    bool mipmap = true;
    TextureFormat textureFormat = TextureFormat::None;

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, filter);
        ArchiveDumpNVP(ar, wrap);
        ArchiveDumpNVP(ar, mipmap);
        ArchiveDumpNVP(ar, textureFormat);
    }
};

class OD_API Texture2D: public Resource{
    friend class Graphics;
    friend class Material;
    friend class OpenGLGraphicsDevice;
    friend class WebGPUGraphicsDevice;
public:
    Texture2D();
    ~Texture2D();

    static Ref<Texture2D> CreateFromFile(const std::string& filePath, Texture2DSetting settings); 
    static Ref<Texture2D> CreateFromFileMemory(void* data, size_t size, Texture2DSetting settings, const std::string& label = ""); 
    static Ref<Texture2D> CreateFromRaw(void* data, int width, int height, TextureDataType dataType, Texture2DSetting settings, const std::string& label = ""); 
    static Ref<Texture2D> CreateFromPackage(const char* path, Package& package, Texture2DSetting settings); 
    static Ref<Texture2D> LoadDefautlTexture2D();
    static Ref<Texture2D> CreateBrdfLUTTexture2D();

    void SetLoadSettings(Texture2DSetting inloadSettings);
    bool LoadFromFileMemory(void* data, size_t size, const std::string& label = ""); 
    
    bool LoadFromFile(const std::string& path) override;
    bool LoadFromPackage(const std::string& path, Package& package) override;
    
    std::vector<std::string> GetFileAssociations() override;

    bool IsValid();
    unsigned int Width();
    unsigned int Height();

    //inline Texture2DSetting& GetLoadSettings(){ return loadSettings; }//Info: Temp add function
    //inline Texture2DSetting GetSettings() const { return settings; }//Info: Temp add function
    
    void* RenderId();

    void OnGui() override;
    void Reload() override;
    bool Save(const std::string& outPath, SaveType type) override;

    bool GetPixelData(std::vector<uint8_t>& outData);

    static void CreateLuaBind(sol::state& lua);

    template <class Archive>
    void serialize(Archive& ar){
        if constexpr(std::is_same_v<Archive, cereal::BinaryOutputArchive>){
            SaveTo(ar);
        } else if constexpr(std::is_same_v<Archive, cereal::BinaryInputArchive>){
            LoadFrom(ar);
        } else {
            Assert(false && "Not Supported");
        }
    }

    virtual size_t RamUsage() override { return ramUsage; }
    virtual size_t VRamUsage() override { return vramUsage; }

private:
    unsigned int width = 0;
    unsigned int height = 0;
    size_t ramUsage = 0;
    size_t vramUsage = 0;
    bool mipmap = false;
    Texture2DSetting loadSettings{};
    Texture2DSetting settings{};
    bool isComplete = false;
    Texture2DDataGL;
    Texture2DDataWG;

    Gfx::Texture2D tex = Gfx::InvalidID;

    void SaveTo(cereal::BinaryOutputArchive& ar);
    void LoadFrom(cereal::BinaryInputArchive& ar);
};

class OD_API Texture2DArray: public Resource{
    friend class Graphics;
    friend class OpenGLGraphicsDevice;
public:
    Texture2DArray(const std::vector<std::string>& filePaths); 
    ~Texture2DArray();

    inline unsigned int Width(){ return width; }
    inline unsigned int Height(){ return height; }

    void OnGui() override;

private:
    unsigned int width;
    unsigned int height;
    Texture2DArrayDataGL;
};

}