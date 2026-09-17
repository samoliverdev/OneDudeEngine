#include "OpenglGPU.h"
#include "OD/Gfx/GfxReflection.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Application.h"
#include "OD/Platform/BaseGpu/MultithreadRendererContext.h"
#include <imgui/backends/imgui_impl_opengl3.h>
#include <string>
#include <regex>
#include <optional>
#include <algorithm>

#define OPENGL_CHECK_ERRORS 1

namespace OD{
namespace Gfx{  

//#define DONT_DEFERRED_RESOURCE_CREATION

#if OPENGL_CHECK_ERRORS
    #define glCheckError() glCheckError_(__FILE__, __LINE__)
    #define glCheckError2(...) glCheckError_(__FILE__, __LINE__, __VA_ARGS__)
#else
    #define glCheckError()
    #define glCheckError2(...)
#endif

int glCheckError_(const char *file, int line, std::function<void()> callback = nullptr){
    //CheckVRAM_AMD();

    GLenum errorCode;
    while((errorCode = glGetError()) != GL_NO_ERROR){
        std::string error = "OTHER";
        switch (errorCode){
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            //case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            //case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            //case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }

        //std::cout << error << " | " << file << " (" << line << ")" << std::endl;
        LogFatal("OpenGL:ERROR: {}({}) | {} ({})\n", error.c_str(), errorCode, file, line);
        //Assert(false);
        if(callback != nullptr) callback();

        Assert(false);
    }

    return errorCode;
}

////////////////////////////////////

#pragma region Pipeline
enum class ShaderBindingType
{
    UniformBuffer,
    Sampler2D,
    SamplerCube
};

struct ShaderBinding{
    uint32_t set = 0;
    uint32_t binding = 0;

    std::string name;
    ShaderBindingType type;
};

std::optional<ShaderBinding> ParseLayoutBinding(std::string& line){
    static const std::regex regex(
        R"(layout\s*\(\s*set\s*=\s*(\d+)\s*,\s*binding\s*=\s*(\d+)\s*\)\s*)"
        R"(uniform\s+([A-Za-z_][A-Za-z0-9_]*)(?:\s+([A-Za-z_][A-Za-z0-9_]*))?)"
    );

    std::smatch match;

    if(!std::regex_search(line, match, regex))
        return std::nullopt;

    ShaderBinding result;

    result.set = static_cast<uint32_t>(std::stoul(match[1]));
    result.binding = static_cast<uint32_t>(std::stoul(match[2]));

    const std::string type = match[3].str();

    // Determine binding type.
    if(type == "sampler2D"){
        result.type = ShaderBindingType::Sampler2D;
    } else if(type == "samplerCube"){
        result.type = ShaderBindingType::SamplerCube;
    } else {
        // Anything else is treated as a uniform buffer.
        result.type = ShaderBindingType::UniformBuffer;
    }

    // For:
    //
    // uniform CameraBuffer
    //
    // match[4] is empty.
    //
    // For:
    //
    // uniform sampler2D Albedo
    //
    // match[4] = Albedo.
    //
    if(result.type == ShaderBindingType::UniformBuffer)
        result.name = type;
    else
        result.name = match[4].str();

    // Find the beginning of "uniform".
    const size_t uniformPos = line.find("uniform", match.position(0));

    if(uniformPos != std::string::npos){
        // Remove only:
        //
        // layout(set = X, binding = X)
        //
        // and preserve the uniform declaration.
        line.erase(
            match.position(0),
            uniformPos - match.position(0)
        );

        // Uniform buffers need std140 on OpenGL.
        if(result.type == ShaderBindingType::UniformBuffer){
            line.insert(0, "layout(std140) ");
        }
    }

    return result;
}

std::string ProcessShaderSource(std::string shaderSource, std::vector<ShaderBinding>& bindings){
    std::stringstream input(shaderSource);
    std::string line;

    std::string output;

    while(std::getline(input, line)){
        auto binding = ParseLayoutBinding(line);

        if(binding){
            /*LogInfo("set:     {}", binding->set);
            LogInfo("binding: {}", binding->binding);
            LogInfo("name:    {}", binding->name);*/

            bindings.push_back(binding.value());
        }

        output += line;
        output += '\n';
    }

    return output;
}

bool OpenglGPUDevice::_CreatePipeline(PipelineData& data, const char* _source, const PipelineInfo& info){
    int  success;
    char infoLog[512];

    data.info = info;

    //std::string source = std::string(cmd.createPipeline.source);

    //std::vector<ShaderBinding> bindings;
    std::string source = _source; //ProcessShaderSource(cmd.createPipeline.source, bindings);
    //auto out = ProcessShaderSource(source, bindings);
    //LogInfo("-----------------\n{}-------------------\n", out);

    Gfx::ShaderReflection reflection;
    Gfx::Reflect(_source, reflection);

    std::string vertexSource =
        "#version 460 core\n"
        "#define OpenGL_API\n"
        "#define OpenGL_API_New\n"
        "#define UseUniformBuffer\n"
        "#define VERTEX\n" +
        source;

    std::string fragmentSource =
        "#version 460 core\n"
        "#define OpenGL_API\n"
        "#define OpenGL_API_New\n"
        "#define UseUniformBuffer\n"
        "#define FRAGMENT\n" +
        source;


    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);  
    const GLchar* vCStr = vertexSource.c_str();
    glShaderSource(vertexShader, 1, &vCStr, 0);
    glCompileShader(vertexShader);  
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(success == GL_FALSE){
        GLint maxLength = 0;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);
        glCheckError();

        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);
        glCheckError();

        glDeleteShader(vertexShader);
        glCheckError();

        //printf("%s", infoLog.data());
        //Assert(false && "Shader compilation failure!");
        LogError("Shader compilation failure!");
        LogError("{}", infoLog.data());
    }
    Assert(success);

    unsigned int fragShader;
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);  
    const GLchar* fCStr = fragmentSource.c_str();
    glShaderSource(fragShader, 1, &fCStr, 0);
    glCompileShader(fragShader);  
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if(success == GL_FALSE){
        GLint maxLength = 0;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);
        glCheckError();

        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);
        glCheckError();

        glDeleteShader(vertexShader);
        glCheckError();

        //printf("%s", infoLog.data());
        //Assert(false && "Shader compilation failure!");
        LogError("Shader compilation failure!");
        LogError("{}", infoLog.data());
    }
    Assert(success);

    //unsigned int shaderProgram;
    data.program = glCreateProgram();

    glAttachShader(data.program, vertexShader);
    glAttachShader(data.program, fragShader);
    glLinkProgram(data.program);

    glGetProgramiv(data.program, GL_LINK_STATUS, &success);
    Assert(success);

    glUseProgram(data.program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragShader); 

    glCheckError();

    for(int i = 0; i < reflection.bindings.size(); i++){
        const auto& _bind = reflection.bindings[i];

        if(_bind.type == BindingType::UniformBuffer){
            GLuint blockIndex = glGetUniformBlockIndex(data.program, _bind.blockName.c_str());
            glCheckError();

            Assert(blockIndex != GL_INVALID_INDEX);
            data.groupsLookUp[_bind.set].bindingsLookUp[_bind.binding] = blockIndex;
        }

        if(_bind.type == BindingType::Texture2D || _bind.type == BindingType::TextureCube){
            GLint uniformLoc = glGetUniformLocation(data.program, _bind.name.c_str());
            glCheckError();

            Assert(uniformLoc >= 0);
            data.groupsLookUp[_bind.set].bindingsLookUp[_bind.binding] = uniformLoc;
        }
    }

    /*for(int i = 0; i < bindings.size(); i++){
        if(bindings[i].type == ShaderBindingType::UniformBuffer){
            GLuint blockIndex = glGetUniformBlockIndex(pipelinePool.Get(cmd.createPipeline.id).program, bindings[i].name.c_str());
            glCheckError();

            Assert(blockIndex != GL_INVALID_INDEX);
            pipelinePool.Get(cmd.createPipeline.id).groupsLookUp[bindings[i].set].bindingsLookUp[bindings[i].binding] = blockIndex;
        }

        if(bindings[i].type == ShaderBindingType::Sampler2D){
            GLint uniformLoc = glGetUniformLocation(pipelinePool.Get(cmd.createPipeline.id).program, bindings[i].name.c_str());
            glCheckError();

            Assert(uniformLoc >= 0);
            pipelinePool.Get(cmd.createPipeline.id).groupsLookUp[bindings[i].set].bindingsLookUp[bindings[i].binding] = uniformLoc;
        }
    }*/
    
    return true;
}

void OpenglGPUDevice::_DestroyPipeline(PipelineData& data){
    
}
#pragma endregion

#pragma region Buffer
GLenum GetBufferTarget(BufferUsage usage){
    switch(usage){
        case BufferUsage::Vertex: return GL_ARRAY_BUFFER;
        case BufferUsage::Index: return GL_ELEMENT_ARRAY_BUFFER;
        case BufferUsage::Uniform: return GL_UNIFORM_BUFFER;
        case BufferUsage::Storage: return GL_SHADER_STORAGE_BUFFER;
    }
    Assert(false);
    return GL_ARRAY_BUFFER;
}

GLenum GetOpenGLBufferUsage(BufferMemory memory){
    switch(memory){
        case BufferMemory::GPUOnly: return GL_STATIC_DRAW;
        case BufferMemory::CPUToGPU: return GL_DYNAMIC_DRAW;
        case BufferMemory::GPUToCPU: return GL_DYNAMIC_READ;
        case BufferMemory::CPUOnly: return GL_STREAM_DRAW;
    }

    Assert(false);
    return GL_STATIC_DRAW;
}

bool OpenglGPUDevice::_CreateBuffer(BufferData& data, size_t size, BufferUsage usage, BufferMemory memory){
    glGenBuffers(1, &data.buffer);
    glCheckError();
    data.type = GetBufferTarget(usage);
    data.usage = usage;
    data.memory = memory;
    return true;
}   

void OpenglGPUDevice::_UpdatedBuffer(BufferData& data, const void* _data, size_t size){
    glBindBuffer(data.type, data.buffer);
    glBufferData(data.type, size, _data, GetOpenGLBufferUsage(data.memory));
    glCheckError();
}

void OpenglGPUDevice::_DestroyBuffer(BufferData& data){
    glDeleteBuffers(1, &data.buffer);
    glCheckError();
    data.buffer = 0;
}
#pragma endregion

#pragma region Texture2D

GLenum GetTextureDataType(ImageFormat f){
    switch(f){
        case ImageFormat::R8_UNORM: return GL_UNSIGNED_BYTE;

        case ImageFormat::R8G8B8_UNORM: return GL_UNSIGNED_BYTE;
        case ImageFormat::R8G8B8_SRGB: return GL_UNSIGNED_BYTE;

        case ImageFormat::R8G8B8A8_UNORM: return GL_UNSIGNED_BYTE;
        case ImageFormat::R8G8B8A8_SRGB: return GL_UNSIGNED_BYTE;
    }

    Assert(false);
    return GL_INVALID_ENUM;
}

GLenum GetTextureInternalFormat(ImageFormat f){
    switch(f){
        case ImageFormat::R8_UNORM: return GL_R8;

        case ImageFormat::R8G8B8_UNORM: return GL_RGB8;
        case ImageFormat::R8G8B8_SRGB: return GL_SRGB8;

        case ImageFormat::R8G8B8A8_UNORM: return GL_RGBA8;
        case ImageFormat::R8G8B8A8_SRGB: return GL_SRGB8_ALPHA8;
    }

    Assert(false);
    return GL_INVALID_ENUM;
}

GLint GetTextureFormat(ImageFormat f){
    switch(f){
        case ImageFormat::R8_UNORM: return GL_RED;

        case ImageFormat::R8G8B8_UNORM: return GL_RGB;
        case ImageFormat::R8G8B8_SRGB: return GL_RGB;

        case ImageFormat::R8G8B8A8_UNORM: return GL_RGBA;
        case ImageFormat::R8G8B8A8_SRGB: return GL_RGBA;
    }

    Assert(false);
    return GL_INVALID_VALUE;
}

GLenum ToGLTextureFilter(TextureFilter filter, bool mipmapped = false){
    if(mipmapped)
        return filter == TextureFilter::Nearest ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
    return filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
}

GLenum ToGLTextureWrapping(TextureWrapping wrapping){
    switch(wrapping){
        case TextureWrapping::Repeat:         return GL_REPEAT;
        case TextureWrapping::MirroredRepeat: return GL_MIRRORED_REPEAT;
        case TextureWrapping::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrapping::ClampToBorder:  return GL_CLAMP_TO_BORDER;
    }
    return GL_REPEAT;
}


bool OpenglGPUDevice::_CreateTexture2D(Texture2DData& texData, const Texture2DInfo& info){
    texData.info = info;

    glGenTextures(1, &texData.tex);  
    glBindTexture(GL_TEXTURE_2D, texData.tex);  
    glCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ToGLTextureWrapping(info.wrapping));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ToGLTextureWrapping(info.wrapping));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ToGLTextureFilter(info.filter, info.mipmap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ToGLTextureFilter(info.filter));
    glCheckError();

    glTexImage2D(
        GL_TEXTURE_2D, 0, 
        GetTextureInternalFormat(texData.info.format), 
        texData.info.width, texData.info.height, 0, 
        GetTextureFormat(texData.info.format), 
        GetTextureDataType(texData.info.format), 
        nullptr
    );
    glCheckError();
    if(texData.info.mipmap) glGenerateMipmap(GL_TEXTURE_2D);
    glCheckError();

    return true;
} 

void OpenglGPUDevice::_UploadTexture2D(Texture2DData& texData, const void* data, size_t size){
    glBindTexture(GL_TEXTURE_2D, texData.tex);
    glCheckError();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texData.info.width, texData.info.height, GetTextureFormat(texData.info.format), GetTextureDataType(texData.info.format), data);
    glCheckError();
    if(texData.info.mipmap) glGenerateMipmap(GL_TEXTURE_2D);
    glCheckError();
}

void OpenglGPUDevice::_DestroyTexture2D(Texture2DData& data){

} 

bool OpenglGPUDevice::_CreateCubemap(CubemapData& data, const CubemapInfo& info){
    data.info = info;
    glGenTextures(1, &data.tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, data.tex);
    for(int face = 0; face < 6; ++face){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0,
            GetTextureInternalFormat(info.format), info.width, info.height, 0,
            GetTextureFormat(info.format), GetTextureDataType(info.format), nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, ToGLTextureFilter(info.filter, info.mipmap));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, ToGLTextureFilter(info.filter));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, ToGLTextureWrapping(info.wrapping));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, ToGLTextureWrapping(info.wrapping));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, ToGLTextureWrapping(info.wrapping));
    if(info.mipmap) glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    return true;
}

void OpenglGPUDevice::_UploadCubemap(CubemapData& data, const void* rawData, size_t size){
    const size_t faceSize = size / 6;
    const size_t expected = static_cast<size_t>(data.info.width) * data.info.height * 4 * 6;
    if(size < expected) return;
    glBindTexture(GL_TEXTURE_CUBE_MAP, data.tex);
    for(int face = 0; face < 6; ++face){
        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, 0, 0,
            data.info.width, data.info.height, GetTextureFormat(data.info.format),
            GetTextureDataType(data.info.format), static_cast<const uint8_t*>(rawData) + face * faceSize);
        glCheckError();
    }
    if(data.info.mipmap) glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glCheckError();
}

void OpenglGPUDevice::_DestroyCubemap(CubemapData& data){
    if(data.tex != 0) glDeleteTextures(1, &data.tex);
    data.tex = 0;
}
#pragma endregion

#pragma region BindGroupLayout
bool OpenglGPUDevice::_CreateBindGroupLayout(BindGroupLayoutData& data, BindGroupLayoutInfo& info){
    data.info = info;
    return true;
}

void OpenglGPUDevice::_DestroyBindGroupLayout(BindGroupLayoutData& data){

}
#pragma endregion

#pragma region BindGroup
bool OpenglGPUDevice::_CreateBindGroup(BindGroupData& data, BindGroupInfo& info){
    data.info = info;
    return true;
}
#pragma endregion

#pragma region Framebuffer
GLenum ToGLInternalFormat(FramebufferTextureFormat format){
    switch(format){
        case FramebufferTextureFormat::RGB: return GL_RGB8;
        case FramebufferTextureFormat::RGBA8: return GL_RGBA8;
        case FramebufferTextureFormat::RGB11B10F: return GL_R11F_G11F_B10F;
        case FramebufferTextureFormat::RGB16F: return GL_RGB16F;
        case FramebufferTextureFormat::RGBA16F: return GL_RGBA16F;
        case FramebufferTextureFormat::RGB32F: return GL_RGB32F;
        case FramebufferTextureFormat::RGBA32F: return GL_RGBA32F;
        case FramebufferTextureFormat::RED_INTEGER: return GL_R32I;
        default: return GL_NONE;
    }
}

GLenum ToGLFormat(FramebufferTextureFormat format){
    switch(format){
        case FramebufferTextureFormat::RGB:
        case FramebufferTextureFormat::RGB11B10F:
        case FramebufferTextureFormat::RGB16F:
        case FramebufferTextureFormat::RGB32F: return GL_RGB;
        case FramebufferTextureFormat::RGBA8:
        case FramebufferTextureFormat::RGBA16F:
        case FramebufferTextureFormat::RGBA32F: return GL_RGBA;
        case FramebufferTextureFormat::RED_INTEGER: return GL_RED_INTEGER;
        default:
            return GL_NONE;
    }
}

GLenum ToGLType(FramebufferTextureFormat format){
    switch(format){
        case FramebufferTextureFormat::RED_INTEGER: return GL_INT;
        case FramebufferTextureFormat::RGB11B10F: return GL_UNSIGNED_INT_10F_11F_11F_REV;
        case FramebufferTextureFormat::RGB16F:
        case FramebufferTextureFormat::RGBA16F:
        case FramebufferTextureFormat::RGB32F:
        case FramebufferTextureFormat::RGBA32F: return GL_FLOAT;
        default: return GL_UNSIGNED_BYTE;
    }
}

GLenum ToGLDepthInternalFormat(FramebufferDepthTextureFormat format){
    switch(format){
        case FramebufferDepthTextureFormat::DEPTH24_STENCIL8: return GL_DEPTH24_STENCIL8;
        case FramebufferDepthTextureFormat::DEPTH32F_STENCIL8: return GL_DEPTH32F_STENCIL8;
        case FramebufferDepthTextureFormat::DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT16;
        case FramebufferDepthTextureFormat::DEPTH_COMPONENT24: return GL_DEPTH_COMPONENT24;
        case FramebufferDepthTextureFormat::DEPTH_COMPONENT32: return GL_DEPTH_COMPONENT32;
        case FramebufferDepthTextureFormat::DEPTH_COMPONENT32F: return GL_DEPTH_COMPONENT32F;
        default: return GL_NONE;
    }
}

bool OpenglGPUDevice::_CreateFramebuffer(FramebufferData& data, const FrameBufferCreateInfo& info){
    #undef max
    data.layout = info.layout;
    data.width = info.width;
    data.height = info.height;

    // ------------------------------------------------------------
    // Create framebuffer
    // ------------------------------------------------------------
    glGenFramebuffers(1, &data.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, data.framebuffer);

    const bool isArray = info.layout.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY;
    const bool isCube = info.layout.type == FramebufferAttachmentType::CUBEMAP;
    const uint32_t layerCount = isCube ? 6u : isArray ? math::max<uint8_t>(1u, info.layout.layers) : 1u;
    if(isCube) Assert(info.width == info.height);

    // ------------------------------------------------------------
    // Color attachments
    // ------------------------------------------------------------
    data.colorAttachments.resize(info.layout.colorAttachmentsCount);

    std::vector<GLenum> drawBuffers;
    drawBuffers.reserve(info.layout.colorAttachmentsCount);

    for(uint32_t i = 0; i < info.layout.colorAttachmentsCount; ++i){
        const FramebufferAttachment& attachment = info.layout.colorAttachments[i];

        //OpenGLFramebufferAttachment& texture = data.colorAttachments[i];
        //texture.format = attachment.format;

        GLenum internalFormat = ToGLInternalFormat(attachment.format);
        GLenum format = ToGLFormat(attachment.format);
        GLenum type = ToGLType(attachment.format);

        Assert(internalFormat != GL_NONE);

        // --------------------------------------------------------
        // Create texture
        // --------------------------------------------------------
        glGenTextures(1, &data.colorAttachments[i]);
        const GLenum target = isArray ? GL_TEXTURE_2D_ARRAY : isCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        glBindTexture(target, data.colorAttachments[i]);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, ToGLTextureFilter(info.filter, attachment.mipLevels > 1));
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, ToGLTextureFilter(info.filter));
        glTexParameteri(target, GL_TEXTURE_WRAP_S, ToGLTextureWrapping(info.wrapping));
        glTexParameteri(target, GL_TEXTURE_WRAP_T, ToGLTextureWrapping(info.wrapping));
        if(isCube) glTexParameteri(target, GL_TEXTURE_WRAP_R, ToGLTextureWrapping(info.wrapping));

        // --------------------------------------------------------
        // Allocate texture
        // --------------------------------------------------------
        if(isArray){
            glTexImage3D(target, 0, internalFormat, static_cast<GLsizei>(info.width), static_cast<GLsizei>(info.height), static_cast<GLsizei>(layerCount), 0, format, type, nullptr);
        } else if(isCube){
            const uint32_t mipCount = std::max(1u, static_cast<uint32_t>(attachment.mipLevels));
            glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mipCount - 1));
            for(uint32_t mip = 0; mip < mipCount; ++mip){
                const GLsizei mipWidth = std::max<GLsizei>(1, static_cast<GLsizei>(info.width >> mip));
                const GLsizei mipHeight = std::max<GLsizei>(1, static_cast<GLsizei>(info.height >> mip));
                for(uint32_t face = 0; face < 6; ++face){
                    glTexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                        static_cast<GLint>(mip),
                        internalFormat,
                        mipWidth,
                        mipHeight,
                        0,
                        format,
                        type,
                        nullptr
                    );
                }
            }
        } else {
            glTexImage2D(target, 0, internalFormat, static_cast<GLsizei>(info.width), static_cast<GLsizei>(info.height), 0, format, type, nullptr);
        }

        // --------------------------------------------------------
        // Generate mipmaps if requested
        // --------------------------------------------------------
        if(attachment.mipLevels > 1 && !isCube){
            glGenerateMipmap(target);
        }

        // --------------------------------------------------------
        // Attach texture to framebuffer
        // --------------------------------------------------------
        if(isArray)
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, data.colorAttachments[i], 0, 0);
        else
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, isCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D, data.colorAttachments[i], 0);
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }

    // ------------------------------------------------------------
    // Depth attachment
    // ------------------------------------------------------------
    if(info.layout.depthAttachment.format != FramebufferDepthTextureFormat::None){
        const FramebufferDepthAttachment& attachment = info.layout.depthAttachment;
        //OpenGLFramebufferDepthAttachment& texture = data.depthAttachment;
        //texture.format = attachment.format;

        GLenum internalFormat = ToGLDepthInternalFormat(attachment.format);
        Assert(internalFormat != GL_NONE);

        // --------------------------------------------------------
        // Create depth texture
        // --------------------------------------------------------
        glGenTextures(1, &data.depthAttachment);
        const GLenum target = isArray ? GL_TEXTURE_2D_ARRAY : isCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        glBindTexture(target, data.depthAttachment);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, ToGLTextureFilter(info.filter, attachment.mipLevels > 1));
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, ToGLTextureFilter(info.filter));
        glTexParameteri(target, GL_TEXTURE_WRAP_S, ToGLTextureWrapping(info.wrapping));
        glTexParameteri(target, GL_TEXTURE_WRAP_T, ToGLTextureWrapping(info.wrapping));
        if(isCube) glTexParameteri(target, GL_TEXTURE_WRAP_R, ToGLTextureWrapping(info.wrapping));

        // --------------------------------------------------------
        // Allocate depth texture
        // --------------------------------------------------------
        const bool hasStencil = attachment.format == FramebufferDepthTextureFormat::DEPTH24_STENCIL8 ||
            attachment.format == FramebufferDepthTextureFormat::DEPTH32F_STENCIL8;
        const GLenum format = hasStencil ? GL_DEPTH_STENCIL : GL_DEPTH_COMPONENT;
        const GLenum type = attachment.format == FramebufferDepthTextureFormat::DEPTH24_STENCIL8
            ? GL_UNSIGNED_INT_24_8
            : attachment.format == FramebufferDepthTextureFormat::DEPTH32F_STENCIL8
                ? GL_FLOAT_32_UNSIGNED_INT_24_8_REV
                : attachment.format == FramebufferDepthTextureFormat::DEPTH_COMPONENT16
                    ? GL_UNSIGNED_SHORT
                    : attachment.format == FramebufferDepthTextureFormat::DEPTH_COMPONENT24
                        ? GL_UNSIGNED_INT
                        : GL_FLOAT;

        if(isArray)
            glTexImage3D(target, 0, internalFormat, static_cast<GLsizei>(info.width), static_cast<GLsizei>(info.height), static_cast<GLsizei>(layerCount), 0, format, type, nullptr);
        else if(isCube){
            const uint32_t mipCount = std::max(1u, static_cast<uint32_t>(attachment.mipLevels));
            glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mipCount - 1));
            for(uint32_t mip = 0; mip < mipCount; ++mip){
                const GLsizei mipWidth = std::max<GLsizei>(1, static_cast<GLsizei>(info.width >> mip));
                const GLsizei mipHeight = std::max<GLsizei>(1, static_cast<GLsizei>(info.height >> mip));
                for(uint32_t face = 0; face < 6; ++face){
                    glTexImage2D(
                        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                        static_cast<GLint>(mip),
                        internalFormat,
                        mipWidth,
                        mipHeight,
                        0,
                        format,
                        type,
                        nullptr
                    );
                }
            }
        } else
            glTexImage2D(target, 0, internalFormat, static_cast<GLsizei>(info.width), static_cast<GLsizei>(info.height), 0, format, type, nullptr);

        // --------------------------------------------------------
        // Attach depth
        // --------------------------------------------------------
        const GLenum depthTarget = hasStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
        if(isArray)
            glFramebufferTextureLayer(GL_FRAMEBUFFER, depthTarget, data.depthAttachment, 0, 0);
        else
            glFramebufferTexture2D(GL_FRAMEBUFFER, depthTarget, isCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D, data.depthAttachment, 0);
        
        if(attachment.mipLevels > 1 && !isCube){
            glGenerateMipmap(target);
        }
    }

    // ------------------------------------------------------------
    // Configure draw buffers
    // ------------------------------------------------------------
    if(drawBuffers.empty()){
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    } else {
        glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
    }

    // ------------------------------------------------------------
    // Check framebuffer
    // ------------------------------------------------------------
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    Assert(status == GL_FRAMEBUFFER_COMPLETE && "OpenGL framebuffer is incomplete");

    glBindTexture(isArray ? GL_TEXTURE_2D_ARRAY : isCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void OpenglGPUDevice::_DestroyFramebuffer(FramebufferData& data){
    if(data.framebuffer != 0){
        glDeleteFramebuffers(1, &data.framebuffer);
        data.framebuffer = 0;
    }
    if(!data.colorAttachments.empty()){
        glDeleteTextures(static_cast<GLsizei>(data.colorAttachments.size()), data.colorAttachments.data());
        data.colorAttachments.clear();
    }
    if(data.depthAttachment != 0){
        glDeleteTextures(1, &data.depthAttachment);
        data.depthAttachment = 0;
    }
}
#pragma endregion

#pragma region Device
GraphicsStats& OpenglGPUDevice::GetStats(){ return _GraphicsStats; }
GPUMemoryStats& OpenglGPUDevice::GetMemoryStats(){ return _GPUMemoryStats; }
GraphicsDebug& OpenglGPUDevice::GetGraphicsDebug(){ return _GraphicsDebug; }

GraphicsDeviceInfo OpenglGPUDevice::GetInfo(){
    return info;
}

OpenglGPUDevice::OpenglGPUDevice(){
    info.apiName = "OpenGL";
    info.version = 4;
    info.supportUniformBuffer = true;

    windowFrameBufferLayout.colorAttachments[0].format = FramebufferTextureFormat::RGBA8;
    windowFrameBufferLayout.colorAttachmentsCount = 1;
    windowFrameBufferLayout.depthAttachment.format = FramebufferDepthTextureFormat::DEPTH24_STENCIL8;
    windowFrameBufferLayout.swapChainTarget = true;
}

FrameBufferLayout OpenglGPUDevice::GetWindowFrameBufferLayout(){
    return windowFrameBufferLayout;
}

void OpenglGPUDevice::LoadContext(void* data){
    LogInfo("OpenGLGraphicsDevice::LoadContext");
    #ifdef __EMSCRIPTEN__
    #else
    gladLoadGLLoader((GLADloadproc)data);
    #endif
}

void OpenglGPUDevice::_Init(){
    LogInfo("OpenglGPUDevice::Initialize");
    ImGuiInitialize();
    glViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    LogInfo("Opengl Version: {}", (char*)glGetString(GL_VERSION));
    LogInfo("GL_VENDOR: {}", (char*)glGetString(GL_VENDOR));
    LogInfo("GL_RENDERER: {}", (char*)glGetString(GL_RENDERER));
    LogInfo("GL_SHADING_LANGUAGE_VERSION: {}", (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    glGenVertexArrays(1, &globalVAO);
	glBindVertexArray(globalVAO);
    glCheckError();

    /*std::string shaderSource = R"GLSL(
    layout(location = 0) in vec3 aPos;
    layout(location = 7) in vec3 aColor;

    layout(set = 0, binding = 0) uniform CameraBuffer{
        mat4 view;
        mat4 proj;
        mat4 viewproj;
    } cameraData;
    layout(set = 0, binding = 2) uniform sampler2D test; 
    layout(set = 1, binding = 0) uniform samplerCube test2; 
    )GLSL";

    std::vector<ShaderBinding> bindings;
    auto out = ProcessShaderSource(shaderSource, bindings);
    LogInfo("-----------------\n{}-------------------\n", out);*/

    /*std::string line = "layout       (     set        =    0,       binding =       0      )   uniform         CameraBuffer";

    auto binding = ParseLayoutBinding(line);
    if(binding){
        LogInfo("set:     {}", binding->set);
        LogInfo("binding: {}", binding->binding);
        LogInfo("name:    {}", binding->name);
        LogInfo("line:    {}", line);
    }*/
}

void OpenglGPUDevice::_Shut(){
    LogInfo("OpenglGPUDevice::Shut");
    cubemapPool.ForEach([&](uint32_t, CubemapData& data){
        if(data.tex != 0) _DestroyCubemap(data);
    });
    ImGuiShutdown();
}

void OpenglGPUDevice::ImGuiInitialize(){
    ImGui_ImplOpenGL3_Init("#version 460");
    // Build the font atlas before the main thread calls ImGui::NewFrame().
    // Subsequent backend NewFrame calls remain on the render thread.
    ImGui_ImplOpenGL3_NewFrame();
}

void OpenglGPUDevice::ImGuiNewFrame(){
    ImGui_ImplOpenGL3_NewFrame();
}

void OpenglGPUDevice::SubmitImGuiDrawData(void* data, ImGuiDrawDataDestroyFunction destroy){
    auto& frame = *multithreadRendererContext.simulationFrame;
    frame.RecordImGuiDrawData(data, destroy);
}

void OpenglGPUDevice::ImGuiShutdown(){
    ImGui_ImplOpenGL3_Shutdown();
}

void OpenglGPUDevice::Init(bool inmultithread){
    multithread = inmultithread;

    multithreadRendererContext.StartupFrames();

    if(multithread){
        multithreadRendererContext.init = [&](){ _Init(); }; //_Init;
        multithreadRendererContext.shut = [&](){ _Shut(); }; //_Shut;
        multithreadRendererContext.runRender = [&](RenderFrame& f){ RunRender(f); f.Clear(); Platform::SwapBuffers(); };
        multithreadRendererContext.Init();
    } else {
        _Init();
    }
}

void OpenglGPUDevice::Shut(){
    if(multithread){
        multithreadRendererContext.Shut();
    } else {
        _Shut();
    }
}

void OpenglGPUDevice::StartRender(){
    if(multithread){
        multithreadRendererContext.StartRender();
    } else {
        SyncSingleThreadData();
    }
}

void OpenglGPUDevice::UpdateRender(){
    if(multithread){
        multithreadRendererContext.WaitForRender();
        multithreadRendererContext.SwapRenderFrames();
        SyncSingleThreadData();
    } else {
        RunRender(*multithreadRendererContext.simulationFrame);
        multithreadRendererContext.simulationFrame->Clear();
        Platform::SwapBuffers();
    }
}

CommandBuffer* OpenglGPUDevice::GetCommandBuffer(){ 
    return &multithreadRendererContext.simulationFrame->renderCommands; 
}

////////////////////////////////////////////////

/*
GLuint GetVertexLocation(VertexSemantic semantic){
    switch(semantic){
        case VertexSemantic::Position:   return 0;
        case VertexSemantic::Normal:     return 1;
        case VertexSemantic::Tangent:    return 2;

        case VertexSemantic::UV0:        return 3;
        case VertexSemantic::UV1:        return 4;
        case VertexSemantic::UV2:        return 5;
        case VertexSemantic::UV3:        return 6;

        case VertexSemantic::Color0:     return 7;
        case VertexSemantic::Color1:     return 8;

        case VertexSemantic::Weights:    return 9;
        case VertexSemantic::Influences: return 10;

        case VertexSemantic::Custom0:    return 11;
        case VertexSemantic::Custom1:    return 12;
        case VertexSemantic::Custom2:    return 13;
        case VertexSemantic::Custom3:    return 14;
    }

    Assert(false);
    return 0;
}
*/

void ApplyVertexAttribute(GLuint location, VertexFormat format, size_t offset, size_t stride){
    switch(format){
        case VertexFormat::Float:
            glVertexAttribPointer(location, 1, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            break;

        case VertexFormat::Float2:
            glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            break;

        case VertexFormat::Float3:
            glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            break;

        case VertexFormat::Float4:
            glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            break;

        case VertexFormat::Int:
            glVertexAttribPointer(location, 1, GL_INT, GL_FALSE, stride, (void*)offset);
            break;

        case VertexFormat::Int2:
            glVertexAttribPointer(location, 2, GL_INT, GL_FALSE, stride, (void*)offset);
            break;

        case VertexFormat::Int3:
            glVertexAttribPointer(location, 3, GL_INT, GL_FALSE, stride, (void*)offset);
            break;

        case VertexFormat::Int4:
            glVertexAttribPointer(location, 4, GL_INT, GL_FALSE, stride, (void*)offset);
            break;

        default:
            Assert(false);
            break;
    }
}

void OpenglGPUDevice::RunRender(RenderFrame& frame){
    #undef max

    Pipeline currentPipeline = INVALID_ID;
    PipelineInfo currentPipelineInfo = {};

    int curBindIndex = 0;
    int curTextureIndex = 0;

    /*for(auto i: frameBindGroups){
        bindGroupPool.AddDestroyedId(i);
    }
    frameBindGroups.clear();*/

    for(const ResourceCommands::Command& cmd: frame.resourceCommands.commands){
        switch(cmd.type){
        case ResourceCommands::Type::CreateBuffer:{
            Assert(bufferPool.IsValid(cmd.createBuffer.id));
            auto& data = bufferPool.Get(cmd.createBuffer.id);
            if(!_CreateBuffer(data, cmd.createBuffer.size, cmd.createBuffer.usage, cmd.createBuffer.memory)){
                bufferPool.AddDestroyedId(cmd.createBuffer.id);
            }
            //bufferPool.gpuToCpuResourceStatesIds.push_back(cmd.createBuffer.id);
            //bufferPool.gpuToCpuResourceStatesData.push_back({ResourceStatsType::Created});
            break;
        }

        case ResourceCommands::Type::UpdateBuffer:{
            Assert(bufferPool.IsValid(cmd.updateBuffer.id));
            auto& data = bufferPool.Get(cmd.updateBuffer.id);
            _UpdatedBuffer(data, cmd.updateBuffer.data, cmd.updateBuffer.size);
            break;
        }

        case ResourceCommands::Type::DestroyBuffer:{
            if(!bufferPool.IsValid(cmd.destroyBuffer.id)){
                LogError("DestroyBuffer: Invalid id!");
                break;
            }

            Assert(bufferPool.IsValid(cmd.destroyBuffer.id));
            auto& data = bufferPool.Get(cmd.destroyBuffer.id);
            _DestroyBuffer(data);
            bufferPool.AddDestroyedId(cmd.destroyBuffer.id);
            break;
        }

        case ResourceCommands::Type::CreateTexture2D:{
            Assert(texture2DPool.IsValid(cmd.createTexture2D.id));
            auto& data = texture2DPool.Get(cmd.createTexture2D.id);
            if(!_CreateTexture2D(data, cmd.createTexture2D.info)){
                texture2DPool.AddDestroyedId(cmd.createTexture2D.id);
            }
            break;
        }

        case ResourceCommands::Type::UploadTexture2D:{
            Assert(texture2DPool.IsValid(cmd.uploadTexture2D.id));
            auto& data = texture2DPool.Get(cmd.uploadTexture2D.id);
            _UploadTexture2D(data, cmd.uploadTexture2D.data, cmd.uploadTexture2D.size);
            break;
        }

        case ResourceCommands::Type::DestroyTexture2D:{
            Assert(texture2DPool.IsValid(cmd.destroyTexture2D.id));
            auto& data = texture2DPool.Get(cmd.destroyTexture2D.id);
            _DestroyTexture2D(data);
            texture2DPool.AddDestroyedId(cmd.destroyTexture2D.id);
            break;
        }

        case ResourceCommands::Type::CreateCubemap:{
            Assert(cubemapPool.IsValid(cmd.createCubemap.id));
            auto& data = cubemapPool.Get(cmd.createCubemap.id);
            if(!_CreateCubemap(data, cmd.createCubemap.info)) cubemapPool.AddDestroyedId(cmd.createCubemap.id);
            break;
        }
        case ResourceCommands::Type::UploadCubemap:{
            Assert(cubemapPool.IsValid(cmd.uploadCubemap.id));
            _UploadCubemap(cubemapPool.Get(cmd.uploadCubemap.id), cmd.uploadCubemap.data, cmd.uploadCubemap.size);
            break;
        }
        case ResourceCommands::Type::DestroyCubemap:{
            Assert(cubemapPool.IsValid(cmd.destroyCubemap.id));
            _DestroyCubemap(cubemapPool.Get(cmd.destroyCubemap.id));
            cubemapPool.AddDestroyedId(cmd.destroyCubemap.id);
            break;
        }

        case ResourceCommands::Type::CreatePipeline:{
            Assert(pipelinePool.IsValid(cmd.createPipeline.id));
            auto& data = pipelinePool.Get(cmd.createPipeline.id);
            if(!_CreatePipeline(data, cmd.createPipeline.source, cmd.createPipeline.info)){
                pipelinePool.AddDestroyedId(cmd.createPipeline.id);
            }
            break;
        }

        case ResourceCommands::Type::DestroyPipeline:{
            Assert(pipelinePool.IsValid(cmd.destroyPipeline.id));
            auto& data = pipelinePool.Get(cmd.destroyPipeline.id);
            _DestroyPipeline(data);
            pipelinePool.AddDestroyedId(cmd.destroyPipeline.id);
            break;
        }
        
        case ResourceCommands::Type::CreateBindGroupLayout:{
            Assert(bindGroupLayoutPool.IsValid(cmd.createBindGroupLayout.id));
            BindGroupLayoutData& data = bindGroupLayoutPool.Get(cmd.createBindGroupLayout.id);
            if(!_CreateBindGroupLayout(data, *cmd.createBindGroupLayout.info)){
                bindGroupLayoutPool.AddDestroyedId(cmd.createBindGroupLayout.id);
            }
            break;
        }

        case ResourceCommands::Type::DestroyBindGroupLayout:{
            Assert(bindGroupLayoutPool.IsValid(cmd.destroyBindGroupLayout.id));
            BindGroupLayoutData& data = bindGroupLayoutPool.Get(cmd.destroyBindGroupLayout.id);
            _DestroyBindGroupLayout(data);
            bindGroupLayoutPool.AddDestroyedId(cmd.destroyBindGroupLayout.id);
            break;
        }

        case ResourceCommands::Type::CreateBindGroup:{
            Assert(bindGroupPool.IsValid(cmd.createBindGroup.id));
            auto& data = bindGroupPool.Get(cmd.createBindGroup.id);
            _CreateBindGroup(data, *cmd.createBindGroup.info);
            break;
        }

        case ResourceCommands::Type::CreateFrameBindGroup:{
            Assert(bindGroupPool.IsValid(cmd.createFrameBindGroup.id));
            auto& data = bindGroupPool.Get(cmd.createFrameBindGroup.id);
            _CreateBindGroup(data, *cmd.createFrameBindGroup.info);
            frameBindGroups.push_back(cmd.createFrameBindGroup.id);
            break;
        }

        case ResourceCommands::Type::CreateFramebuffer:{
            Assert(framebufferPool.IsValid(cmd.createFramebuffer.framebuffer));
            auto& data = framebufferPool.Get(cmd.createFramebuffer.framebuffer);
            if(!_CreateFramebuffer(data, cmd.createFramebuffer.info)){
                framebufferPool.AddDestroyedId(cmd.createFramebuffer.framebuffer);
            }
            break;
        }

        case ResourceCommands::Type::DestroyFramebuffer:{
            Assert(framebufferPool.IsValid(cmd.destroyFramebuffer.id));
            auto& data = framebufferPool.Get(cmd.destroyFramebuffer.id);
            _DestroyFramebuffer(data);
            framebufferPool.AddDestroyedId(cmd.destroyFramebuffer.id);
            break;
        }

        }
    }

    for(const CommandBuffer::Command& cmd: frame.renderCommands.commands){
        switch(cmd.type){

        case CommandBuffer::Type::Clear:{
            const auto& clear = cmd.clear;
            GLbitfield mask = 0;

            if(HasFlag(clear.flags, ClearFlags::Color)){
                glClearColor(clear.clearValue.color.r, clear.clearValue.color.g, clear.clearValue.color.b, clear.clearValue.color.a);
                mask |= GL_COLOR_BUFFER_BIT;
            }

            if(HasFlag(clear.flags, ClearFlags::Depth)){
                glClearDepth(clear.clearValue.depth);
                mask |= GL_DEPTH_BUFFER_BIT;
            }

            if(HasFlag(clear.flags, ClearFlags::Stencil)){
                glClearStencil(static_cast<GLint>(clear.clearValue.stencil));
                mask |= GL_STENCIL_BUFFER_BIT;
            }

            if(mask != 0) glClear(mask);

            glCheckError();
            break;
        }

        case CommandBuffer::Type::Viewport:{
            glViewport(cmd.viewport.x, cmd.viewport.y, cmd.viewport.w, cmd.viewport.h);
            glCheckError();
            break;
        }

        case CommandBuffer::Type::SetPipeline:{
            PipelineData& pipeline = pipelinePool.Get(cmd.setPipeline.id);
            glUseProgram(pipeline.program);
            currentPipeline = cmd.setPipeline.id;
            currentPipelineInfo = pipeline.info;
            glCheckError();

            curBindIndex = 0;
            curTextureIndex = 0;
            break;
        }

        case CommandBuffer::Type::SetVertexBuffer:{
            Assert(bufferPool.Get(cmd.setVertexBuffer.buffer).usage == BufferUsage::Vertex);

            const uint32_t slot = cmd.setVertexBuffer.slot;
            GLuint vbo = bufferPool.Get(cmd.setVertexBuffer.buffer).buffer;
            const MeshLayout& layout = currentPipelineInfo.vertexLayout;
            const VertexBufferLayout& bufferLayout = layout.buffers[slot];

            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            for(uint32_t i = 0; i < layout.attributeCount; ++i){
                const VertexAttribute& attribute = layout.attributes[i];
                if(attribute.bufferSlot != slot) continue;

                GLuint location = VertexSemanticToSlot(attribute.semantic); // GetVertexLocation(attribute.semantic);
                ApplyVertexAttribute(location, attribute.format, attribute.offset, bufferLayout.stride);
                glEnableVertexAttribArray(location);
                glVertexAttribDivisor(location, bufferLayout.inputRate == VertexInputRate::Instance ? 1 : 0);
            }

            glCheckError();
            break;
        }

        case CommandBuffer::Type::SetIndexBuffer:{
            Assert(bufferPool.Get(cmd.setIndexBuffer.buffer).usage == BufferUsage::Index);

            GLuint ebo = bufferPool.Get(cmd.setIndexBuffer.buffer).buffer;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            break;
        };

        case CommandBuffer::Type::SetBindGroup:{
            //Assert(group < bindGroups.size());

            const BindGroupData& bindGroup = bindGroupPool.Get(cmd.setBindGroup.group);
            const BindGroupLayoutData& bindGroupLayout = bindGroupLayoutPool.Get(bindGroup.info.layout);

            for(int i = 0; i < bindGroup.info.entriesCount; i++){
                if(bindGroupLayout.info.entries[i].type == BindingType::UniformBuffer){
                    const BindingEntry& binding = bindGroup.info.entries[i];
                    Assert(binding.buffer != InvalidID);

                    const BufferData& buffer = bufferPool.Get(binding.buffer);
                    Assert(buffer.usage == BufferUsage::Uniform);

                    Assert(buffer.buffer != InvalidID);
                    Assert(binding.dynamicOffset == false);

                    const PipelineData& pipeline = pipelinePool.Get(currentPipeline);
                    GLuint blockIndex = pipeline.groupsLookUp[cmd.setBindGroup.slot].bindingsLookUp[binding.binding];

                    glBindBufferRange(
                        GL_UNIFORM_BUFFER,
                        curBindIndex,
                        buffer.buffer,
                        static_cast<GLintptr>(binding.offset),
                        static_cast<GLsizeiptr>(binding.size)
                    );
                    glCheckError();
                    glUniformBlockBinding(pipeline.program, blockIndex, curBindIndex);
                    glCheckError();

                    curBindIndex += 1;
                }

                if(bindGroupLayout.info.entries[i].type == BindingType::Texture2D){
                    const BindingEntry& binding = bindGroup.info.entries[i];
                    Assert(binding.buffer != InvalidID);

                    if(binding.texture != InvalidID){
                        const Texture2DData& tex = texture2DPool.Get(binding.texture);

                        const PipelineData& pipeline = pipelinePool.Get(currentPipeline);
                        GLuint uniformLoc = pipeline.groupsLookUp[cmd.setBindGroup.slot].bindingsLookUp[binding.binding];

                        glActiveTexture(GL_TEXTURE0 + curTextureIndex);
                        glBindTexture(GL_TEXTURE_2D, tex.tex);
                        glUniform1i(uniformLoc, curTextureIndex); // set it manually
                        glCheckError();

                        curTextureIndex += 1;
                    }
                    
                    if(binding.framebuffer != InvalidID){
                        const FramebufferData& tex = framebufferPool.Get(binding.framebuffer);

                        const PipelineData& pipeline = pipelinePool.Get(currentPipeline);
                        GLuint uniformLoc = pipeline.groupsLookUp[cmd.setBindGroup.slot].bindingsLookUp[binding.binding];

                        glActiveTexture(GL_TEXTURE0 + curTextureIndex);
                        glBindTexture(GL_TEXTURE_2D, binding.framebufferAttacement < 0 ? tex.depthAttachment : tex.colorAttachments[binding.framebufferAttacement]);
                        glUniform1i(uniformLoc, curTextureIndex); // set it manually
                        glCheckError();

                        curTextureIndex += 1;
                    }
                }

                if(bindGroupLayout.info.entries[i].type == BindingType::TextureCube){
                    const BindingEntry& binding = bindGroup.info.entries[i];
                    if(binding.cubemap != InvalidID){
                        const CubemapData& cube = cubemapPool.Get(binding.cubemap);
                        const PipelineData& pipeline = pipelinePool.Get(currentPipeline);
                        GLuint uniformLoc = pipeline.groupsLookUp[cmd.setBindGroup.slot].bindingsLookUp[binding.binding];
                        glActiveTexture(GL_TEXTURE0 + curTextureIndex);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, cube.tex);
                        glUniform1i(uniformLoc, curTextureIndex);
                        curTextureIndex += 1;
                    }
                }
            }
            break;
        }

        case CommandBuffer::Type::Draw:{
            glDrawArrays(GL_TRIANGLES, 0, cmd.draw.vertexCount);
            glCheckError();
            break;
        }

        case CommandBuffer::Type::DrawIndexed:{
            glDrawElements(GL_TRIANGLES, cmd.drawIndexed.indexCount, GL_UNSIGNED_INT, nullptr);
            glCheckError();
            break;
        }

        case CommandBuffer::Type::DrawInstanced:{
            glDrawArraysInstanced(GL_TRIANGLES, 0, cmd.drawInstanced.vertexCount, cmd.drawInstanced.count);
            break;
        }

        case CommandBuffer::Type::DrawIndexedInstanced:{
            glDrawElementsInstanced(GL_TRIANGLES, cmd.drawIndexedInstanced.indexCount,  GL_UNSIGNED_INT, 0, cmd.drawIndexedInstanced.count);
            break;
        }

        case CommandBuffer::Type::BeginWindowFramebuffer:{
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glCheckError();
            break;
        }

        case CommandBuffer::Type::BeginFramebuffer:{
            FramebufferData& data = framebufferPool.Get(cmd.beginFramebuffer.framebuffer);

            Assert(data.framebuffer > 0);
            glBindFramebuffer(GL_FRAMEBUFFER, data.framebuffer);
            const bool isArray = data.layout.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY;
            const bool isCube = data.layout.type == FramebufferAttachmentType::CUBEMAP;
            const uint32_t layerCount = isCube ? 6u : isArray ? math::max<uint8_t>(1u, data.layout.layers) : 1u;
            const uint32_t layer = cmd.beginFramebuffer.layer;
            const uint32_t mip = cmd.beginFramebuffer.mip;
            Assert(layer < layerCount);

            for(uint32_t i = 0; i < data.layout.colorAttachmentsCount; ++i){
                if(isArray)
                    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, data.colorAttachments[i], mip, layer);
                else if(isCube)
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, data.colorAttachments[i], mip);
            }
            if(data.depthAttachment != 0){
                const bool hasStencil = data.layout.depthAttachment.format == FramebufferDepthTextureFormat::DEPTH24_STENCIL8 ||
                    data.layout.depthAttachment.format == FramebufferDepthTextureFormat::DEPTH32F_STENCIL8;
                const GLenum attachment = hasStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
                if(isArray)
                    glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, data.depthAttachment, mip, layer);
                else if(isCube)
                    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, data.depthAttachment, mip);
            }
            glCheckError();
            break;
        }

        case CommandBuffer::Type::EndFramebuffer:{
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glCheckError();
            break;
        }

        case CommandBuffer::Type::BlitFramebuffer:{
            const auto& blit = cmd.blitFramebuffer;
            Assert(framebufferPool.IsValid(blit.src));
            const auto& src = framebufferPool.Get(blit.src);
            const GLuint dst = blit.dst == InvalidID ? 0 : [&](){
                Assert(framebufferPool.IsValid(blit.dst));
                return framebufferPool.Get(blit.dst).framebuffer;
            }();

            glBindFramebuffer(GL_READ_FRAMEBUFFER, src.framebuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst);
            if(blit.srcPass < 0){
                glBlitFramebuffer(0, 0, src.width, src.height, 0, 0,
                    blit.dst == InvalidID ? src.width : framebufferPool.Get(blit.dst).width,
                    blit.dst == InvalidID ? src.height : framebufferPool.Get(blit.dst).height,
                    GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            } else {
                glReadBuffer(GL_COLOR_ATTACHMENT0 + blit.srcPass);
                // The default framebuffer does not have GL_COLOR_ATTACHMENT*;
                // its color buffer is selected with GL_BACK.
                glDrawBuffer(blit.dst == InvalidID ? GL_BACK : GL_COLOR_ATTACHMENT0 + blit.srcPass);
                const uint32_t dstWidth = blit.dst == InvalidID ? src.width : framebufferPool.Get(blit.dst).width;
                const uint32_t dstHeight = blit.dst == InvalidID ? src.height : framebufferPool.Get(blit.dst).height;
                glBlitFramebuffer(0, 0, src.width, src.height, 0, 0, dstWidth, dstHeight,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
            }
            glCheckError();
            break;
        }

        case CommandBuffer::Type::RenderImGui:{
            const uint32_t index = cmd.renderImGui.snapshotIndex;
            Assert(index < frame.imguiSnapshots.size());

            const auto& snapshot = frame.imguiSnapshots[index];
            if(snapshot.data == nullptr)
                break;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
            ImGuiNewFrame();
            ImGui_ImplOpenGL3_RenderDrawData(static_cast<ImDrawData*>(snapshot.data));
            break;
        }

        }
    }

    for(auto i: frameBindGroups){
        bindGroupPool.AddDestroyedId(i);
    }
    frameBindGroups.clear();

}

void OpenglGPUDevice::SyncSingleThreadData(){
    bufferPool.SyncSingleThreadData();
    pipelinePool.SyncSingleThreadData();
    bindGroupPool.SyncSingleThreadData();
    cubemapPool.SyncSingleThreadData();
}

///////////////////////////////////

Pipeline OpenglGPUDevice::CreatePipeline(const char* source, PipelineInfo info){   
    auto id = pipelinePool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreatePipeline(id, source, info);
    return id;
}

void OpenglGPUDevice::DestroyPipeline(Pipeline id){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyPipeline(id);
}

Buffer OpenglGPUDevice::CreateBuffer(size_t size, BufferUsage usage, BufferMemory memory){
    #ifdef DONT_DEFERRED_RESOURCE_CREATION

    BufferData data;
    if(!_CreateBuffer(data, size, usage, memory)) return InvalidID;
    auto id = bufferPool.AllocId();
    bufferPool.CpuPushResource(id, data);
    return id;

    #else

    auto id = bufferPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateBuffer(id, size, usage, memory);
    return id;

    #endif
}

void OpenglGPUDevice::UpdatedBuffer(Buffer buffer, const void* data, size_t size){
    multithreadRendererContext.simulationFrame->resourceCommands.UpdatedBuffer(buffer, data, size);
}

void OpenglGPUDevice::DestroyBuffer(Buffer id){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyBuffer(id);
}

Texture2D OpenglGPUDevice::CreateTexture2D(Texture2DInfo& info){
    #ifdef DONT_DEFERRED_RESOURCE_CREATION

    Texture2DData data;
    if(!_CreateTexture2D(data, info)) return InvalidID;
    auto id = texture2DPool.AllocId();
    texture2DPool.CpuPushResource(id, data);
    return id;

    #else

    auto id = texture2DPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateTexture2D(id, info);
    return id;

    #endif
}

void OpenglGPUDevice::UploadTexture2D(Texture2D texture, const void* data, size_t size){
    multithreadRendererContext.simulationFrame->resourceCommands.UploadTexture2D(texture, data, size);
}

void OpenglGPUDevice::DestroyTexture2D(Texture2D tex){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyTexture2D(tex);
}

Cubemap OpenglGPUDevice::CreateCubemap(CubemapInfo& info){
    #ifdef DONT_DEFERRED_RESOURCE_CREATION

    CubemapData data{};
    if(!_CreateCubemap(data, info)) return InvalidID;
    auto id = cubemapPool.AllocId();
    cubemapPool.CpuPushResource(id, data);
    return id;

    #else

    auto id = cubemapPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateCubemap(id, info);
    return id;

    #endif
}

void OpenglGPUDevice::UploadCubemap(Cubemap cubemap, const void* data, size_t size){
    multithreadRendererContext.simulationFrame->resourceCommands.UploadCubemap(cubemap, data, size);
}

void OpenglGPUDevice::DestroyCubemap(Cubemap cubemap){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyCubemap(cubemap);
}

BindGroupLayout OpenglGPUDevice::CreateBindGroupLayout(BindGroupLayoutInfo& info){
    #ifdef DONT_DEFERRED_RESOURCE_CREATION

    BindGroupLayoutData data;
    if(!_CreateBindGroupLayout(data, info)) return InvalidID;
    auto id = bindGroupLayoutPool.AllocId();
    bindGroupLayoutPool.CpuPushResource(id, data);
    return id;

    #else

    auto id = bindGroupLayoutPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateBindGroupLayout(id, info);
    return id;

    #endif
}

void OpenglGPUDevice::DestroyBindGroupLayout(BindGroupLayout layout){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyBindGroupLayout(layout);
}

BindGroup OpenglGPUDevice::CreateBindGroup(BindGroupInfo& info){
    auto id = bindGroupPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateBindGroup(id, info);
    return id;
}

BindGroup OpenglGPUDevice::CreateFrameBindGroup(BindGroupInfo& info){
    auto id = bindGroupPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateFrameBindGroup(id, info);
    return id;
}

Framebuffer OpenglGPUDevice::CreateFramebuffer(FrameBufferCreateInfo& info){ 
    auto id = framebufferPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateFramebuffer(id, info);
    return id;
}

void OpenglGPUDevice::DestroyFramebuffer(Framebuffer framebuffer){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyFramebuffer(framebuffer);
}

ResourceStats OpenglGPUDevice::GetBufferStats(Buffer id){ 
    return bufferPool.GetStatus(id);// .resourceStatus[id]; 
}

#pragma endregion
}
}
