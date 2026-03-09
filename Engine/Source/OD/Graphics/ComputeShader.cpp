#include "OD/pch.h"
#include "ComputeShader.h"
#include "GraphicsDevice.h"

namespace OD{

extern GraphicsDevice* graphicsDevice;

ComputeShader::ComputeShader(){

}

ComputeShader::~ComputeShader(){

}

bool ComputeShader::LoadFromFile(const std::string& path){
    std::ifstream file(path);
    if(!file.is_open()){
        LogError("ComputeShader: Failed to open {}", path);
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();

    return graphicsDevice->ComputeShaderCreate(*this, ss.str());
}

std::vector<std::string> ComputeShader::GetFileAssociations(){
    return {".compute"};
}

void ComputeShader::Dispatch(uint32_t x, uint32_t y, uint32_t z){
    graphicsDevice->ComputeShaderDispatch(*this, x, y, z);
}

void ComputeShader::SetTexture(const char* name, Ref<Texture2D> tex){
    graphicsDevice->ComputeShaderSetTexture(*this, name, tex);
}

void ComputeShader::SetTexture(const char* name, Framebuffer* tex, int attachment){
    graphicsDevice->ComputeShaderSetTexture(*this, name, tex, attachment);
}

void ComputeShader::SetUniformBuffer(const char* name, Ref<UniformBuffer> buffer, int bind){
    graphicsDevice->ComputeShaderSetUniformBuffer(*this, name, buffer, bind);
}

void ComputeShader::SetComputeBuffer(const char* name, Ref<ComputeBuffer> buffer, int bind){
    graphicsDevice->ComputeShaderSetComputeBuffer(*this, name, buffer, bind);
}

void ComputeShader::SetInt(const char* name, int v){
    graphicsDevice->ComputeShaderSetInt(*this, name, v);
}

void ComputeShader::SetFloat(const char* name, float v){
    graphicsDevice->ComputeShaderSetFloat(*this, name, v);
}

void ComputeShader::SetVector4(const char* name, Vector4 v){
    graphicsDevice->ComputeShaderSetVector4(*this, name, v);
}

bool ComputeShader::IsValid(){
    return graphicsDevice->ComputeShaderIsValid(*this);
}

}