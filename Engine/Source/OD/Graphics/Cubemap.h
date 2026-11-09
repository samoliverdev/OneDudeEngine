#pragma once
#include "OD/Core/Resource.h"
#include "OD/Platform/OpenGL/GL.h"
#include "OD/Gfx/Gfx.h"

namespace sol{ class state; }

namespace OD{

class Graphics;

class OD_API Cubemap: public Resource{
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
#ifdef TestNewGPU_API
    bool LoadFromHDRFile(const char* hdri);
#endif
    bool mipmap = false;
    size_t ramUsage = 0;
    size_t vramUsage = 0;
    CubemapDataGL; 
    Gfx::Cubemap tex = Gfx::InvalidID;
};

}
