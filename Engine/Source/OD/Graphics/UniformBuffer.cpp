#include "OD/pch.h"
#include "UniformBuffer.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "OD/Defines.h"

namespace OD{

extern GraphicsDevice* graphicsDevice;
extern Gfx::Device* gfxDevice;

Ref<UniformBuffer> UniformBuffer::Create(size_t size){
    Ref<UniformBuffer> buffer = CreateRef<UniformBuffer>(size);
    return buffer;

    /*Ref<UniformBuffer> buffer = CreateRef<UniformBuffer>();

    glGenBuffers(1, &buffer->rendererId);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer->rendererId);
    glCheckError();
    
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glCheckError();
    
    return buffer;*/
}

UniformBuffer::UniformBuffer(size_t size){
    #ifdef TestNewGPU_API
    buffer = gfxDevice->CreateBuffer(size, Gfx::BufferUsage::Uniform, Gfx::BufferMemory::CPUToGPU);
    //cpuData.resize(size);
    #else
    graphicsDevice->UniformBufferCreate(*this, size);
    #endif
}

UniformBuffer::~UniformBuffer(){
    #ifdef TestNewGPU_API
    //Assert(false);
    gfxDevice->DestroyBuffer(buffer);
    #else
    graphicsDevice->UniformBufferDestroy(*this);
    #endif
}

//void UniformBuffer::Destroy(){
    //graphicsDevice->UniformBufferDestroy(*this);

    /*if(buffer.rendererId != 0) glDeleteBuffers(1, &buffer.rendererId);
    buffer.rendererId = 0;
    glCheckError();*/
//}

bool UniformBuffer::IsValid(){
    #ifdef TestNewGPU_API
    //Assert(false);
    return true;
    #else
    return graphicsDevice->UniformBufferIsValid(*this);
    #endif
}

//void UniformBuffer::Bind(UniformBuffer& buffer, int bind){
    /*glBindBuffer(GL_UNIFORM_BUFFER, buffer.rendererId);
    glBindBufferBase(GL_UNIFORM_BUFFER, bind, buffer.rendererId);
    glCheckError();*/ 
//}

void UniformBuffer::SetData(const void* data, size_t size, size_t offset){
    #ifdef TestNewGPU_API
    Assert(data != nullptr);
    /*Assert(offset + size <= cpuData.size());
    std::memcpy(cpuData.data() + offset, data, size);
    gfxDevice->UpdatedBuffer(buffer, cpuData.data(), cpuData.size());*/
    gfxDevice->UpdatedBuffer(buffer, data, size);
    #else
    graphicsDevice->UniformBufferSetData(*this, data, size, offset);
    #endif

    /*Assert(IsValid() == true);

    glBindBuffer(GL_UNIFORM_BUFFER, rendererId);
    glCheckError();
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
    glCheckError();*/
}

}
