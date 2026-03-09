#pragma once
#include "OD/Core/Asset.h"
#include "OD/Platform/OpenGL/GL.h"

namespace OD{

class OD_API ComputeBuffer {
    friend class Graphics;
    friend class OpenGLGraphicsDevice;
public:
    ComputeBuffer(size_t size);
    ~ComputeBuffer();
    bool IsValid();
    void SetData(const void* data, unsigned int size, unsigned int offset = 0);
    void GetData(void* data, unsigned int size, unsigned int offset = 0);
    template<typename T> void GetData(T* data, unsigned int count){ GetData(data, sizeof(T) * count, 0); }
    size_t VRamUsage(){ return vramUsage; }
private:
    size_t vramUsage;
    ComputeBufferDataGL;
};

}