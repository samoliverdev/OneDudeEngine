#pragma once
#include "OD/Core/Asset.h"
#include "OD/Platform/OpenGL/GL.h"

namespace sol{ class state; }

namespace OD{

class Graphics;

class OD_API Cubemap: public Asset{
    friend class Graphics;
    friend class OpenGLGraphicsDevice;
public:
    Cubemap();
    ~Cubemap();

    static Ref<Cubemap> CreateFromFile(
        const char* right, const char* left, const char* top,
        const char* bottom, const char* front, const char* back
    ); 
    static Ref<Cubemap> CreateFromFileHDR(const char* hdri);
    static Ref<Cubemap> CreateIrradianceMapFromCubeMap(const Ref<Cubemap>& cubemap);  
    static Ref<Cubemap> CreatePrefilterMapFromCubeMap(const Ref<Cubemap>& cubemap);  

    static void CreateLuaBind(sol::state& lua);

    bool LoadFromFile(const std::string& path) override;
    std::vector<std::string> GetFileAssociations() override;

    virtual size_t RamUsage() override { return ramUsage; }
    virtual size_t VRamUsage() override { return vramUsage; }
    
private:
    bool mipmap;
    size_t ramUsage;
    size_t vramUsage;
    CubemapDataGL; 
};

}