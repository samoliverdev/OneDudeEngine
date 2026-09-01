#include "OpenglGPU.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Application.h"
#include "OD/Platform/BaseGpu/ResourcePool.h"
#include <string>
#include <regex>
#include <optional>
#include <index_vector.hpp>
#include <glad.h>

#define OPENGL_CHECK_ERRORS 1

namespace OD{

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

struct BufferData{
    uint32_t buffer = 0;
    GPUBufferUsage usage;
    GPUBufferMemory memory;
};
ResourcePool<BufferData> bufferPool;

struct BindGroupLookUp{
    GLuint bindingsLookUp[20];
};

struct PipelineData{
    uint32_t program = 0;
    GPUPipelineInfo info;
    BindGroupLookUp groupsLookUp[4];
};
ResourcePool<PipelineData> pipelinePool;

struct BindGroupData{
    GPUBindGroupInfo info;
};
ResourcePool<BindGroupData> bindGroupPool;

unsigned int globalVAO = 0;

GraphicsDeviceInfo info;

GraphicsStats _GraphicsStats;
GPUMemoryStats _GPUMemoryStats;
GraphicsDebug _GraphicsDebug;

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

////////////////////////////////////

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

void OpenglGPUDevice::Init(){
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

void OpenglGPUDevice::Shut(){
    LogInfo("OpenglGPUDevice::Shut");
}

////////////////////////////////////////////////

GLuint GetVertexLocation(GPUVertexSemantic semantic){
    switch(semantic){
        case GPUVertexSemantic::Position:   return 0;
        case GPUVertexSemantic::Normal:     return 1;
        case GPUVertexSemantic::Tangent:    return 2;

        case GPUVertexSemantic::UV0:        return 3;
        case GPUVertexSemantic::UV1:        return 4;
        case GPUVertexSemantic::UV2:        return 5;
        case GPUVertexSemantic::UV3:        return 6;

        case GPUVertexSemantic::Color0:     return 7;
        case GPUVertexSemantic::Color1:     return 8;

        case GPUVertexSemantic::Weights:    return 9;
        case GPUVertexSemantic::Influences: return 10;

        case GPUVertexSemantic::Custom0:    return 11;
        case GPUVertexSemantic::Custom1:    return 12;
        case GPUVertexSemantic::Custom2:    return 13;
        case GPUVertexSemantic::Custom3:    return 14;
    }

    Assert(false);
    return 0;
}

void ApplyVertexAttribute(GLuint location, GPUVertexFormat format, size_t offset, size_t stride){
    switch(format){
        case GPUVertexFormat::Float:
            glVertexAttribPointer(location, 1, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            break;

        case GPUVertexFormat::Float2:
            glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            break;

        case GPUVertexFormat::Float3:
            glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            break;

        case GPUVertexFormat::Float4:
            glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            break;

        default:
            Assert(false);
            break;
    }
}

GLenum GetBufferTarget(GPUBufferUsage usage){
    switch(usage){
        case GPUBufferUsage::Vertex: return GL_ARRAY_BUFFER;
        case GPUBufferUsage::Index: return GL_ELEMENT_ARRAY_BUFFER;
        case GPUBufferUsage::Uniform: return GL_UNIFORM_BUFFER;
        case GPUBufferUsage::Storage: return GL_SHADER_STORAGE_BUFFER;
    }
    Assert(false);
    return GL_ARRAY_BUFFER;
}

GLenum GetOpenGLBufferUsage(GPUBufferMemory memory){
    switch(memory){
        case GPUBufferMemory::GPUOnly: return GL_STATIC_DRAW;
        case GPUBufferMemory::CPUToGPU: return GL_DYNAMIC_DRAW;
        case GPUBufferMemory::GPUToCPU: return GL_DYNAMIC_READ;
        case GPUBufferMemory::CPUOnly: return GL_STREAM_DRAW;
    }

    Assert(false);
    return GL_STATIC_DRAW;
}

constexpr bool HasFlag(GPUClearFlags value, GPUClearFlags flag){
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

void OpenglGPUDevice::RunRender(GPURenderFrame& frame){
    PipelineId currentPipeline = INVALID_ID;
    GPUPipelineInfo currentPipelineInfo = {};

    int curBindIndex = 0;

    for(const GPUResourceCommands::Command& cmd: frame.resourceCommands.commands){
        switch(cmd.type){
        case GPUResourceCommands::Type::CreateBuffer:{
            //Assert(cmd.createBuffer.id < bufferPool.data.size());

            if(bufferPool.Get(cmd.createBuffer.id).buffer != 0){
                LogError("Trying CreateBuffer on Used id");
                continue;
            }

            GLuint buffer = 0;
            glGenBuffers(1, &buffer);
            GLenum target = GetBufferTarget(cmd.createBuffer.usage);

            glBindBuffer(target, buffer);
            glBufferData(target, cmd.createBuffer.size, cmd.createBuffer.data, GetOpenGLBufferUsage(cmd.createBuffer.memory));
            bufferPool.Get(cmd.createBuffer.id).buffer = buffer;
            bufferPool.Get(cmd.createBuffer.id).usage = cmd.createBuffer.usage;
            bufferPool.Get(cmd.createBuffer.id).memory = cmd.createBuffer.memory;
            glCheckError();

            bufferPool.gpuToCpuResourceStatesIds.push_back(cmd.createBuffer.id);
            bufferPool.gpuToCpuResourceStatesData.push_back({GPUResourceStatsType::Created});
            break;
        }

        case GPUResourceCommands::Type::DestroyBuffer:{
            Assert(bufferPool.Get(cmd.destroyBuffer.id).buffer != 0);
            glDeleteBuffers(1, &bufferPool.Get(cmd.destroyBuffer.id).buffer);
            glCheckError();
            bufferPool.Get(cmd.destroyBuffer.id).buffer = 0;
            bufferPool.idsDestred.push_back(cmd.destroyBuffer.id);
            break;
        }

        case GPUResourceCommands::Type::CreatePipeline:{
            pipelinePool.Get(cmd.createPipeline.id).info = cmd.createPipeline.info;

            int  success;
            char infoLog[512];

            //std::string source = std::string(cmd.createPipeline.source);

            std::vector<ShaderBinding> bindings;
            std::string source = ProcessShaderSource(cmd.createPipeline.source, bindings);
            auto out = ProcessShaderSource(source, bindings);
            //LogInfo("-----------------\n{}-------------------\n", out);

            std::string vertexSource =
                "#version 460 core\n"
                "#define OpenGL\n"
                "#define Vertex\n" +
                source;

            std::string fragmentSource =
                "#version 460 core\n"
                "#define OpenGL\n"
                "#define Fragment\n" +
                source;

            unsigned int vertexShader;
            vertexShader = glCreateShader(GL_VERTEX_SHADER);  
            const GLchar* vCStr = vertexSource.c_str();
            glShaderSource(vertexShader, 1, &vCStr, 0);
            glCompileShader(vertexShader);  
            glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
            Assert(success);

            unsigned int fragShader;
            fragShader = glCreateShader(GL_FRAGMENT_SHADER);  
            const GLchar* fCStr = fragmentSource.c_str();
            glShaderSource(fragShader, 1, &fCStr, 0);
            glCompileShader(fragShader);  
            glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
            Assert(success);

            //unsigned int shaderProgram;
            pipelinePool.Get(cmd.createPipeline.id).program = glCreateProgram();

            glAttachShader(pipelinePool.Get(cmd.createPipeline.id).program, vertexShader);
            glAttachShader(pipelinePool.Get(cmd.createPipeline.id).program, fragShader);
            glLinkProgram(pipelinePool.Get(cmd.createPipeline.id).program);

            glGetProgramiv(pipelinePool.Get(cmd.createPipeline.id).program, GL_LINK_STATUS, &success);
            Assert(success);

            glUseProgram(pipelinePool.Get(cmd.createPipeline.id).program);
            glDeleteShader(vertexShader);
            glDeleteShader(fragShader); 

            glCheckError();

            for(int i = 0; i < bindings.size(); i++){
                GLuint blockIndex = glGetUniformBlockIndex(pipelinePool.Get(cmd.createPipeline.id).program, bindings[i].name.c_str());
                Assert(blockIndex != GL_INVALID_INDEX);
                pipelinePool.Get(cmd.createPipeline.id).groupsLookUp[bindings[i].set].bindingsLookUp[bindings[i].binding] = blockIndex;
            }
            
            break;
        }

        case GPUResourceCommands::Type::DestroyPipeline:{
            Assert(pipelinePool.Get(cmd.destroyPipeline.id).program != 0);
            glDeleteProgram(pipelinePool.Get(cmd.destroyPipeline.id).program);
            glCheckError();
            pipelinePool.Get(cmd.destroyPipeline.id).program = 0;
            pipelinePool.idsDestred.push_back(cmd.destroyPipeline.id);
            break;
        }
        
        case GPUResourceCommands::Type::CreateBindGroup:{
            std::memcpy(&bindGroupPool.Get(cmd.createBindGroup.id).info, cmd.createBindGroup.info, sizeof(GPUBindGroupInfo));
            break;
        }
        }
    }

    for(const GPUCommandBuffer::Command& cmd: frame.renderCommands.commands){
        switch(cmd.type){

        case GPUCommandBuffer::Type::Clear:{
            const auto& clear = cmd.clear;
            GLbitfield mask = 0;

            if(HasFlag(clear.flags, GPUClearFlags::Color)){
                glClearColor(clear.clearValue.color.r, clear.clearValue.color.g, clear.clearValue.color.b, clear.clearValue.color.a);
                mask |= GL_COLOR_BUFFER_BIT;
            }

            if(HasFlag(clear.flags, GPUClearFlags::Depth)){
                glClearDepth(clear.clearValue.depth);
                mask |= GL_DEPTH_BUFFER_BIT;
            }

            if(HasFlag(clear.flags, GPUClearFlags::Stencil)){
                glClearStencil(static_cast<GLint>(clear.clearValue.stencil));
                mask |= GL_STENCIL_BUFFER_BIT;
            }

            if(mask != 0) glClear(mask);

            glCheckError();
            break;
        }

        case GPUCommandBuffer::Type::Viewport:{
            glViewport(cmd.viewport.x, cmd.viewport.y, cmd.viewport.w, cmd.viewport.h);
            glCheckError();
            break;
        }

        case GPUCommandBuffer::Type::SetPipeline:{
            PipelineData& pipeline = pipelinePool.Get(cmd.setPipeline.id);
            glUseProgram(pipeline.program);
            currentPipeline = cmd.setPipeline.id;
            currentPipelineInfo = pipeline.info;
            glCheckError();

            curBindIndex = 0;
            break;
        }

        case GPUCommandBuffer::Type::SetVertexBuffer:{
            Assert(bufferPool.Get(cmd.setVertexBuffer.buffer).usage == GPUBufferUsage::Vertex);

            const uint32_t slot = cmd.setVertexBuffer.slot;
            GLuint vbo = bufferPool.Get(cmd.setVertexBuffer.buffer).buffer;
            const GPUMeshLayout& layout = currentPipelineInfo.vertexLayout;
            const GPUVertexBufferLayout& bufferLayout = layout.buffers[slot];

            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            for(uint32_t i = 0; i < layout.attributeCount; ++i){
                const GPUVertexAttribute& attribute = layout.attributes[i];
                if(attribute.bufferSlot != slot) continue;

                GLuint location = GetVertexLocation(attribute.semantic);
                ApplyVertexAttribute(location, attribute.format, attribute.offset, bufferLayout.stride);
                glEnableVertexAttribArray(location);
                glVertexAttribDivisor(location, bufferLayout.inputRate == GPUVertexInputRate::Instance ? 1 : 0);
            }

            glCheckError();
            break;
        }

        case GPUCommandBuffer::Type::SetIndexBuffer:{
            Assert(bufferPool.Get(cmd.setIndexBuffer.buffer).usage == GPUBufferUsage::Index);

            GLuint ebo = bufferPool.Get(cmd.setIndexBuffer.buffer).buffer;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            break;
        };

        case GPUCommandBuffer::Type::SetBindGroup:{
            //Assert(group < bindGroups.size());

            const BindGroupData& bindGroup = bindGroupPool.Get(cmd.setBindGroup.group);

            for(int i = 0; i < bindGroup.info.entriesCount; i++){
                const GPUBindingEntry& binding = bindGroup.info.entries[i];
                Assert(binding.buffer != InvalidID);

                const BufferData& buffer = bufferPool.Get(binding.buffer);
                Assert(buffer.usage == GPUBufferUsage::Uniform);

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
                curBindIndex += 1;
                glCheckError();
            }
            break;
        }

        case GPUCommandBuffer::Type::Draw:{
            glDrawArrays(GL_TRIANGLES, 0, cmd.draw.vertexCount);
            glCheckError();
            break;
        }

        case GPUCommandBuffer::Type::DrawIndexed:{
            glDrawElements(GL_TRIANGLES, cmd.drawIndexed.indexCount, GL_UNSIGNED_INT, nullptr);
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

MeshId OpenglGPUDevice::AllocBufferId(){
    return bufferPool.AllocId();
}

PipelineId OpenglGPUDevice::AllocPipelineId(){
    return pipelinePool.AllocId();
}

BindGroupLayoutId OpenglGPUDevice::AllocCreateBindGroupLayoutId(){
    return InvalidID; 
}

BindGroupId OpenglGPUDevice::AllocCreateBindGroupId(){
    return bindGroupPool.AllocId();
}

BufferId OpenglGPUDevice::CreateBuffer(const void* data, size_t size, GPUBufferUsage usage, GPUBufferMemory memory){
    auto id = bufferPool.AllocId();
    BufferData bufferData = {};

    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    GLenum target = GetBufferTarget(usage);

    glBindBuffer(target, buffer);
    glBufferData(target, size, data, GetOpenGLBufferUsage(memory));
    bufferData.buffer = buffer;
    bufferData.usage = usage;
    bufferData.memory = memory;
    glCheckError();

    bufferPool.CpuPushResource(id, bufferData);
    return id;
}

BindGroupLayoutId OpenglGPUDevice::CreateBindGroupLayout(GPUBindGroupLayoutInfo& info){ 
    return InvalidID; 
}

BindGroupId OpenglGPUDevice::CreateBindGroup(GPUBindGroupInfo& info){
    auto id = bindGroupPool.AllocId();
    BindGroupData data = {};
    data.info = info;

    bindGroupPool.CpuPushResource(id, data);
    return id;
}

GPUResourceStats OpenglGPUDevice::GetBufferStats(BufferId id){ 
    return bufferPool.GetStatus(id);// .resourceStatus[id]; 
}

}