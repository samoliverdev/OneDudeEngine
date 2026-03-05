#pragma once
#include "OD/Core/Asset.h"
#include "OD/Platform/OpenGL/GL.h"

namespace OD{

class OD_API UniformBuffer{
    friend class Graphics;
    friend class OpenGLGraphicsDevice;
public:
    static Ref<UniformBuffer> Create();

    UniformBuffer();
    ~UniformBuffer();
    
    bool IsValid();
    void SetData(const void* data, unsigned int size, unsigned int offset = 0);

    size_t VRamUsage(){ return vramUsage; }

    //inline bool IsValid(){ return rendererId != 0; }
    //inline unsigned int RendererId(){ return rendererId; }
    //inline int GetBind(){ return bind; }
private:
    size_t vramUsage;
    UniformBufferDataGL;
    //unsigned int rendererId = 0;
    //int bind = 0;
};

}