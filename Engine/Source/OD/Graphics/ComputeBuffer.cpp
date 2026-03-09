#include "ComputeBuffer.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "OD/Defines.h"

namespace OD{

extern GraphicsDevice* graphicsDevice;

ComputeBuffer::ComputeBuffer(size_t size){
    graphicsDevice->ComputeBufferCreate(*this, size);
}

ComputeBuffer::~ComputeBuffer(){
    graphicsDevice->ComputeBufferDestroy(*this);
}

bool ComputeBuffer::IsValid(){
    return graphicsDevice->ComputeBufferIsValid(*this);
}

void ComputeBuffer::SetData(const void* data, unsigned int size, unsigned int offset){
    graphicsDevice->ComputeBufferSetData(*this, data, size, offset);
}

}