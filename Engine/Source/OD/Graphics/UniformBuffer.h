#pragma once
#include "OD/Core/Resource.h"
#include "OD/Platform/OpenGL/GL.h"

namespace OD{

class OD_API UniformBuffer{
    friend class Graphics;
    friend class OpenGLGraphicsDevice;
public:
    static Ref<UniformBuffer> Create(size_t size);

    UniformBuffer(size_t size);
    ~UniformBuffer();
    
    bool IsValid();
    void SetData(const void* data, size_t size, size_t offset = 0);

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