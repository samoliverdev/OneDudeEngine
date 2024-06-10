#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Platform/GL.h"
#include "OD/Serialization/Serialization.h"
//#include "Shader.h"

namespace OD {

enum class OD_API_IMPORT TextureFilter {
    Nearest,
    Linear
};

class Graphics;

struct OD_API Texture2DSetting{
    TextureFilter filter = TextureFilter::Linear;
    bool mipmap = true;

    template <class Archive>
    void serialize(Archive & ar){
        ar(
            CEREAL_NVP(filter),
            CEREAL_NVP(mipmap)
        );
    }
};

class OD_API Texture2D: public Asset{
    friend class Graphics;
public:
    Texture2D() = default;
    Texture2D(const std::string& filePath, Texture2DSetting settings = Texture2DSetting()); 
    Texture2D(void* data, size_t size, Texture2DSetting settings = Texture2DSetting()); 
    
    void LoadFromFile(const std::string& path) override;
    std::vector<std::string> GetFileAssociations() override;

    static Ref<Texture2D> CreateFromFile(const std::string& filePath, Texture2DSetting settings); 
    static Ref<Texture2D> CreateFromFileMemory(void* data, size_t size, Texture2DSetting settings); 
    static Ref<Texture2D> LoadDefautlTexture2D();
    
    static void Destroy(Texture2D& tex);
    static void Bind(Texture2D& tex, int index);
    
    ~Texture2D();

    bool IsValid();
    inline unsigned int Width(){ return width; }
    inline unsigned int Height(){ return height; }
    inline unsigned int RenderId(){ return id; }

    void OnGui() override;
    void Reload() override;
    void Save() override;

private:
    unsigned int id = 0;
    unsigned int width;
    unsigned int height;
    unsigned int internalFormat;
    unsigned int imageFormat;
    unsigned int wrapS;
    unsigned int wrapT;
    unsigned int filterMin;
    unsigned int filterMax;
    bool mipmap;
    Texture2DSetting settings;

    bool Create(const std::string path, Texture2DSetting settings);
    bool Create(void* data, size_t size, Texture2DSetting settings);
    void texture2DGenerate(unsigned int width, unsigned int height, unsigned char* data);

    //void SaveSettings();
};

class OD_API Texture2DArray: public Asset{
    friend class Graphics;
public:
    Texture2DArray(const std::vector<std::string>& filePaths); 
    ~Texture2DArray();

    static void Destroy(Texture2DArray& tex);
    static void Bind(Texture2DArray& tex, int index);

    bool IsValid();
    inline unsigned int Width(){ return width; }
    inline unsigned int Height(){ return height; }
    inline unsigned int RenderId(){ return id; }

    void OnGui() override;

private:
    unsigned int id = 0;
    unsigned int width;
    unsigned int height;
};

}