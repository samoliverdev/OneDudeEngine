#include "UniformBuffer.h"
#include "OD/Platform/GL.h"
#include "OD/Defines.h"

namespace OD{

Ref<UniformBuffer> UniformBuffer::Create(){
    Ref<UniformBuffer> buffer = CreateRef<UniformBuffer>();

    glGenBuffers(1, &buffer->rendererId);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer->rendererId);
    glCheckError();
    
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glCheckError();
    
    return buffer;
}

void UniformBuffer::Destroy(UniformBuffer& buffer){
    if(buffer.rendererId != 0) glDeleteBuffers(1, &buffer.rendererId);
    buffer.rendererId = 0;
    glCheckError();
}

void UniformBuffer::Bind(UniformBuffer& buffer, int bind){
    glBindBuffer(GL_UNIFORM_BUFFER, buffer.rendererId);
    glBindBufferBase(GL_UNIFORM_BUFFER, bind, buffer.rendererId);
    glCheckError(); 
}

void UniformBuffer::SetData(const void* data, unsigned int size, unsigned int offset){
    Assert(IsValid() == true);

    glBindBuffer(GL_UNIFORM_BUFFER, rendererId);
    glCheckError();

    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
    glCheckError();
}

}