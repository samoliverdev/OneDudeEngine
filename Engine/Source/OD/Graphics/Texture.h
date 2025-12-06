#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Platform/OpenGL/GL.h"
#include "OD/Platform/WebGPU/WebGPU.h"

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
    Auto,
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
    TextureFormat textureFormat = TextureFormat::Auto;

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, filter);
        ArchiveDumpNVP(ar, wrap);
        ArchiveDumpNVP(ar, mipmap);
        ArchiveDumpNVP(ar, textureFormat);
    }
};

class OD_API Texture2D: public Asset{
    friend class Graphics;
    friend class OpenGLGraphicsDevice;
    friend class WebGPUGraphicsDevice;
public:
    Texture2D() = default;
    ~Texture2D();

    static Ref<Texture2D> CreateFromFile(const std::string& filePath, Texture2DSetting settings); 
    static Ref<Texture2D> CreateFromMemory(void* data, size_t size, Texture2DSetting settings, const std::string& label = ""); 
    static Ref<Texture2D> CreateFromRaw(void* data, size_t size, int width, int height, TextureDataType dataType, Texture2DSetting settings, const std::string& label = ""); 
    static Ref<Texture2D> CreateFromPackage(const char* path, Package& package, Texture2DSetting settings); 
    static Ref<Texture2D> LoadDefautlTexture2D();
    static Ref<Texture2D> CreateBrdfLUTTexture2D();
    
    bool LoadFromFile(const std::string& path) override;
    std::vector<std::string> GetFileAssociations() override;

    bool IsValid();
    unsigned int Width();
    unsigned int Height();

    inline Texture2DSetting& GetSettings(){ return settings; }//Info: Temp add function
    
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

private:
    unsigned int width = 0;
    unsigned int height = 0;
    bool mipmap = false;
    Texture2DSetting settings{};
    bool isComplete = false;
    Texture2DDataGL;
    Texture2DDataWG;

    void SaveTo(cereal::BinaryOutputArchive& ar);
    void LoadFrom(cereal::BinaryInputArchive& ar);
};

class OD_API Texture2DArray: public Asset{
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