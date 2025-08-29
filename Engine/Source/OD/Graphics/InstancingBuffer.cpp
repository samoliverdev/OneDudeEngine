#include "InstancingBuffer.h"
#include "Graphics.h"
#include "OD/Defines.h"

namespace OD{

extern GraphicsDevice* graphicsDevice;

Ref<InstancingBuffer> InstancingBuffer::Create(){
    Ref<InstancingBuffer> buffer = CreateRef<InstancingBuffer>();
    if(graphicsDevice->InstancingBufferCreate(*buffer) == false){
        graphicsDevice->InstancingBufferDestroy(*buffer);
        return nullptr;
    }
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
    graphicsDevice->InstancingBufferCreate(*this);
}

InstancingBuffer::~InstancingBuffer(){
    graphicsDevice->InstancingBufferDestroy(*this);
}

//void UniformBuffer::Destroy(){
    //graphicsDevice->UniformBufferDestroy(*this);

    /*if(buffer.rendererId != 0) glDeleteBuffers(1, &buffer.rendererId);
    buffer.rendererId = 0;
    glCheckError();*/
//}

bool InstancingBuffer::IsValid(){
    return graphicsDevice->InstancingBufferIsValid(*this);
}

//void UniformBuffer::Bind(UniformBuffer& buffer, int bind){
    /*glBindBuffer(GL_UNIFORM_BUFFER, buffer.rendererId);
    glBindBufferBase(GL_UNIFORM_BUFFER, bind, buffer.rendererId);
    glCheckError();*/ 
//}

void InstancingBuffer::SetData(const Matrix4* data, unsigned int incount){
    graphicsDevice->InstancingBufferSetData(*this, data, incount);
    count = incount;
}

void InstancingBuffer::SetData(const Matrix4x3* data, unsigned int incount){
    graphicsDevice->InstancingBufferSetData(*this, data, count);
    count = incount;
}

}