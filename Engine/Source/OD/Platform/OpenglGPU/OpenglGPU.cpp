#include "OpenglGPU.h"
#include "OD/Gfx/GfxReflection.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Application.h"
#include "OD/Platform/BaseGpu/MultithreadRendererContext.h"
#include <string>
#include <regex>
#include <optional>

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

        if(_bind.type == BindingType::Texture2D){
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
bool OpenglGPUDevice::_CreateTexture2D(Texture2DData& texData, const Texture2DInfo& info){
    texData.width = info.width;
    texData.height = info.height;

    glGenTextures(1, &texData.tex);  
    glBindTexture(GL_TEXTURE_2D, texData.tex);  

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texData.width, texData.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glGenerateMipmap(GL_TEXTURE_2D);

    return true;
} 

void OpenglGPUDevice::_UploadTexture2D(Texture2DData& texData, const void* data, size_t size){
    glBindTexture(GL_TEXTURE_2D, texData.tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texData.width, texData.height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void OpenglGPUDevice::_DestroyTexture2D(Texture2DData& data){

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
    data.layout = info.layout;
    data.width = info.width;
    data.height = info.height;

    // ------------------------------------------------------------
    // Create framebuffer
    // ------------------------------------------------------------
    glGenFramebuffers(1, &data.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, data.framebuffer);

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
        glBindTexture(GL_TEXTURE_2D, data.colorAttachments[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, attachment.mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // --------------------------------------------------------
        // Allocate texture
        // --------------------------------------------------------
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<GLsizei>(info.width), static_cast<GLsizei>(info.height), 0, format, type, nullptr);

        // --------------------------------------------------------
        // Generate mipmaps if requested
        // --------------------------------------------------------
        if(attachment.mipLevels > 1){
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        // --------------------------------------------------------
        // Attach texture to framebuffer
        // --------------------------------------------------------
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, data.colorAttachments[i], 0);
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
        glBindTexture(GL_TEXTURE_2D, data.depthAttachment);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, attachment.mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // --------------------------------------------------------
        // Allocate depth texture
        // --------------------------------------------------------
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<GLsizei>(info.width), static_cast<GLsizei>(info.height), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        // --------------------------------------------------------
        // Attach depth
        // --------------------------------------------------------
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, data.depthAttachment, 0);
        
        if(attachment.mipLevels > 1){
            glGenerateMipmap(GL_TEXTURE_2D);
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

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void OpenglGPUDevice::_DestroyFramebuffer(FramebufferData& data){

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
    glViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    
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

        default:
            Assert(false);
            break;
    }
}


constexpr bool HasFlag(ClearFlags value, ClearFlags flag){
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

void OpenglGPUDevice::RunRender(RenderFrame& frame){
    Pipeline currentPipeline = INVALID_ID;
    PipelineInfo currentPipelineInfo = {};

    int curBindIndex = 0;
    int curTextureIndex = 0;

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

        case CommandBuffer::Type::BeginWindowFramebuffer:{
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glCheckError();
            break;
        }

        case CommandBuffer::Type::BeginFramebuffer:{
            FramebufferData& data = framebufferPool.Get(cmd.beginFramebuffer.framebuffer);

            Assert(data.framebuffer > 0);
            glBindFramebuffer(GL_FRAMEBUFFER, data.framebuffer);
            glCheckError();
            break;
        }

        case CommandBuffer::Type::EndFramebuffer:{
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glCheckError();
            break;
        }

        }
    }
}

void OpenglGPUDevice::SyncSingleThreadData(){
    bufferPool.SyncSingleThreadData();
    pipelinePool.SyncSingleThreadData();
    bindGroupPool.SyncSingleThreadData();
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
