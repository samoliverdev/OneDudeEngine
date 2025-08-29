#pragma once
#include "OD/Defines.h"
#include "OD/Base.h"
#include "OD/Platform/OpenGL/GL.h"
#include "OD/Core/Math.h"

namespace OD{

class OD_API InstancingBuffer{
    friend class Graphics;
    friend class OpenGLGraphicsDevice;
public:
    static Ref<InstancingBuffer> Create();

    InstancingBuffer();
    ~InstancingBuffer();
    
    bool IsValid();
    //void SetData(const void* data, unsigned int size, unsigned int offset = 0);
    
    void SetData(const Matrix4* data, unsigned int count);
    void SetData(const Matrix4x3* data, unsigned int count);

private:
    bool isMatrix4x3 = false;
    InstancingBufferDataGL;
};

}