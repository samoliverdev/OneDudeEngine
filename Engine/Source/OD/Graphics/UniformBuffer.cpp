#include "UniformBuffer.h"
#include "Graphics.h"
#include "OD/Defines.h"

namespace OD{

extern GraphicsDevice* graphicsDevice;

Ref<UniformBuffer> UniformBuffer::Create(){
    Ref<UniformBuffer> buffer = CreateRef<UniformBuffer>();
    if(graphicsDevice->UniformBufferCreate(*buffer) == false){
        graphicsDevice->UniformBufferDestroy(*buffer);
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

UniformBuffer::UniformBuffer(){
    graphicsDevice->UniformBufferCreate(*this);
}

UniformBuffer::~UniformBuffer(){
    graphicsDevice->UniformBufferDestroy(*this);
}

//void UniformBuffer::Destroy(){
    //graphicsDevice->UniformBufferDestroy(*this);

    /*if(buffer.rendererId != 0) glDeleteBuffers(1, &buffer.rendererId);
    buffer.rendererId = 0;
    glCheckError();*/
//}

bool UniformBuffer::IsValid(){
    return graphicsDevice->UniformBufferIsValid(*this);
}

//void UniformBuffer::Bind(UniformBuffer& buffer, int bind){
    /*glBindBuffer(GL_UNIFORM_BUFFER, buffer.rendererId);
    glBindBufferBase(GL_UNIFORM_BUFFER, bind, buffer.rendererId);
    glCheckError();*/ 
//}

void UniformBuffer::SetData(const void* data, unsigned int size, unsigned int offset){
    graphicsDevice->UniformBufferSetData(*this, data, size, offset);

    /*Assert(IsValid() == true);

    glBindBuffer(GL_UNIFORM_BUFFER, rendererId);
    glCheckError();
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
    glCheckError();*/
}

}