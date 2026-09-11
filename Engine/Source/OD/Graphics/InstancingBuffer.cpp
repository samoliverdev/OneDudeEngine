#include "OD/pch.h"
#include "InstancingBuffer.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "OD/Defines.h"

namespace OD{

extern GraphicsDevice* graphicsDevice;
extern Gfx::Device* gfxDevice;

Ref<InstancingBuffer> InstancingBuffer::Create(){
    Ref<InstancingBuffer> buffer = CreateRef<InstancingBuffer>();
    #ifdef TestNewGPU_API
    //Assert(false);
    #else
    if(graphicsDevice->InstancingBufferCreate(*buffer) == false){
        graphicsDevice->InstancingBufferDestroy(*buffer);
        return nullptr;
    }
    #endif

    return buffer;

    /*Ref<UniformBuffer> buffer = CreateRef<UniformBuffer>();

    glGenBuffers(1, &buffer->rendererId);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer->rendererId);
    glCheckError();
    
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glCheckError();
    
    return buffer;*/
}

InstancingBuffer::InstancingBuffer(){
    #ifdef TestNewGPU_API
    //Assert(false);
    #else
    graphicsDevice->InstancingBufferCreate(*this);
    #endif
}

InstancingBuffer::~InstancingBuffer(){
    #ifdef TestNewGPU_API
    Assert(false);
    if(buffer != Gfx::InvalidID) gfxDevice->DestroyBuffer(buffer);
    #else
    graphicsDevice->InstancingBufferDestroy(*this);
    #endif
}

//void UniformBuffer::Destroy(){
    //graphicsDevice->UniformBufferDestroy(*this);

    /*if(buffer.rendererId != 0) glDeleteBuffers(1, &buffer.rendererId);
    buffer.rendererId = 0;
    glCheckError();*/
//}

bool InstancingBuffer::IsValid(){
    #ifdef TestNewGPU_API
    Assert(false);
    return true;
    #else
    return graphicsDevice->InstancingBufferIsValid(*this);
    #endif
}

//void UniformBuffer::Bind(UniformBuffer& buffer, int bind){
    /*glBindBuffer(GL_UNIFORM_BUFFER, buffer.rendererId);
    glBindBufferBase(GL_UNIFORM_BUFFER, bind, buffer.rendererId);
    glCheckError();*/ 
//}

void InstancingBuffer::SetData(const Matrix4* data, unsigned int incount){
    #ifdef TestNewGPU_API
    //Assert(false);
    if(buffer != Gfx::InvalidID) gfxDevice->DestroyBuffer(buffer);
    buffer = gfxDevice->CreateBuffer(sizeof(Matrix4) * incount, Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    Assert(buffer != Gfx::InvalidID);
    gfxDevice->UpdatedBuffer(buffer, data, sizeof(Matrix4) * incount);
    count = incount;
    #else
    graphicsDevice->InstancingBufferSetData(*this, data, incount);
    count = incount;
    #endif
}

void InstancingBuffer::SetData(const Matrix4x3* data, unsigned int incount){
    #ifdef TestNewGPU_API
    //Assert(false);
    if(buffer != Gfx::InvalidID) gfxDevice->DestroyBuffer(buffer);
    buffer = gfxDevice->CreateBuffer(sizeof(Matrix4x3) * incount, Gfx::BufferUsage::Vertex, Gfx::BufferMemory::GPUOnly);
    Assert(buffer != Gfx::InvalidID);
    gfxDevice->UpdatedBuffer(buffer, data, sizeof(Matrix4x3) * incount);
    count = incount;
    #else
    graphicsDevice->InstancingBufferSetData(*this, data, count);
    count = incount;
    #endif
}

}