#include "OpenglGPU.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Application.h"
#include "OD/Platform/BaseGpu/ResourcePool.h"
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

struct MeshData{
    uint32_t vbo = 0;
};
ResourcePool<MeshData> meshPool;

struct PipelineData{
    uint32_t id = 0;
};
ResourcePool<PipelineData> pipelinePool;

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
}

void OpenglGPUDevice::Shut(){
    LogInfo("OpenglGPUDevice::Shut");
}

void OpenglGPUDevice::RunRender(GPURenderFrame& frame){
    //glClearColor(0, 0, 255, 255);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    for(const GPUCommandBuffer::Command& cmd : frame.commands.commands){
        switch(cmd.type){

        case GPUCommandBuffer::Type::Clean:{
            glClearColor(cmd.cleanColor.r, cmd.cleanColor.g, cmd.cleanColor.b, cmd.cleanColor.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
            glCheckError();
            break;
        }

        case GPUCommandBuffer::Type::Viewport:{
            glViewport(cmd.viewport.x, cmd.viewport.y, cmd.viewport.w, cmd.viewport.h);
            glCheckError();
            break;
        }

        case GPUCommandBuffer::Type::CreateMesh:{
            glGenBuffers(1, &meshPool.data[cmd.createMesh.mesh].vbo);
            glBindBuffer(GL_ARRAY_BUFFER, meshPool.data[cmd.createMesh.mesh].vbo);  
            glBufferData(GL_ARRAY_BUFFER, cmd.createMesh.size, cmd.createMesh.data, GL_STATIC_DRAW);
            glCheckError();
            break;
        }

        case GPUCommandBuffer::Type::DestroyMesh:{
            Assert(meshPool.data[cmd.mesh].vbo != 0);
            glDeleteBuffers(1, &meshPool.data[cmd.mesh].vbo);
            glCheckError();
            meshPool.data[cmd.mesh].vbo = 0;
            meshPool.idsDestred.push_back(cmd.mesh);
            break;
        }

        case GPUCommandBuffer::Type::CreatePipeline:{
            int  success;
            char infoLog[512];

            std::string source = std::string(cmd.createPipeline.data);
            std::string vertexSource =
                "#version 460 core\n"
                "#define Vertex\n" +
                source;

            std::string fragmentSource =
                "#version 460 core\n"
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
            pipelinePool.data[cmd.createPipeline.id].id = glCreateProgram();

            glAttachShader(pipelinePool.data[cmd.createPipeline.id].id, vertexShader);
            glAttachShader(pipelinePool.data[cmd.createPipeline.id].id, fragShader);
            glLinkProgram(pipelinePool.data[cmd.createPipeline.id].id);

            glGetProgramiv(pipelinePool.data[cmd.createPipeline.id].id, GL_LINK_STATUS, &success);
            Assert(success);

            glUseProgram(pipelinePool.data[cmd.createPipeline.id].id);
            glDeleteShader(vertexShader);
            glDeleteShader(fragShader); 

            glCheckError();
            
            break;
        }

        case GPUCommandBuffer::Type::DestroyPipeline:{
            Assert(pipelinePool.data[cmd.pipeline].id != 0);
            glDeleteShader(pipelinePool.data[cmd.pipeline].id);
            glCheckError();
            pipelinePool.data[cmd.pipeline].id = 0;
            pipelinePool.idsDestred.push_back(cmd.pipeline);
            break;
        }

        case GPUCommandBuffer::Type::SetRenderTarget:{
            // glBindFramebuffer(...)
            break;
        }

        case GPUCommandBuffer::Type::SetPipeline:{
            // glUseProgram(...)
            break;
        }

        case GPUCommandBuffer::Type::WriteBuffer:{
            // glNamedBufferSubData(...)
            break;
        }

        case GPUCommandBuffer::Type::Draw:{
            glUseProgram(pipelinePool.data[cmd.draw.pipeline].id);
            glCheckError();

            glBindBuffer(GL_ARRAY_BUFFER, meshPool.data[cmd.draw.mesh].vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);  
            glCheckError();

            glDrawArrays(GL_TRIANGLES, 0, cmd.draw.vertexCount);
            glCheckError();

            // glBindVertexArray(...)
            // glDrawArrays(...)
            break;
        }

        }
    }
}

void OpenglGPUDevice::SyncSingleThreadData(){
    meshPool.SyncSingleThreadData();
    pipelinePool.SyncSingleThreadData();
}

///////////////////////////////////

MeshId OpenglGPUDevice::AllocMeshId(){
    return meshPool.AllocId();
}

PipelineId OpenglGPUDevice::AllocPipelineId(){
    return pipelinePool.AllocId();
}

}