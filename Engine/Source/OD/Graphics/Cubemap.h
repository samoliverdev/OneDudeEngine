#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Platform/OpenGL/GL.h"

namespace sol{ class state; }

namespace OD{

class Graphics;

class OD_API Cubemap: public Asset{
    friend class Graphics;
public:

    static Ref<Cubemap> CreateFromFile(
        const char* right, const char* left, const char* top,
        const char* bottom, const char* front, const char* back
    ); 
    static Ref<Cubemap> CreateFromFileHDR(const char* hdri);
    static Ref<Cubemap> CreateIrradianceMapFromCubeMap(const Ref<Cubemap>& cubemap);  
    static Ref<Cubemap> CreatePrefilterMapFromCubeMap(const Ref<Cubemap>& cubemap);  

    bool IsValid();

    static void CreateLuaBind(sol::state& lua);
    
private:
    bool isComplete = false;
    CubemapDataGL; 
};

}