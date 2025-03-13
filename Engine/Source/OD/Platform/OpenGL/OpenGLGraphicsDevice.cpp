#ifdef OPENGL_SUPPORT
#include "OpenGLGraphicsDevice.h"
#include "GL.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Camera.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Font.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Model.h"
#include "OD/Graphics/SubShader.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Font.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/SerializationFull.h"
#include "OD/Core/Application.h"
#include "OD/Core/ImGui.h"
#include <fstream>
#include <stb/stb_image.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#define UseUniformBuffer 0

namespace OD{

//#define OPENGL_DEBUG

GLenum meshDrawModeLookup[] = {
    GL_TRIANGLES,
    GL_LINES,
    GL_POINTS,
    //GL_QUADS
};  

OpenGLGraphicsDevice::OpenGLGraphicsDevice(){
    info.apiName = "OpenGL";
    info.version = OpenGLVersion;
}

void OpenGLGraphicsDevice::LoadContext(void* data){
    LogInfo("OpenGLGraphicsDevice::LoadContext");
    #ifdef __EMSCRIPTEN__
    #else
    gladLoadGLLoader((GLADloadproc)data);
    #endif
}

GraphicsStats& OpenGLGraphicsDevice::GetStats(){ 
    return stats; 
}

GraphicsDeviceInfo OpenGLGraphicsDevice::GetInfo(){
    return info;
}

#ifdef OPENGL_DEBUG
void DebugCallback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char* message, const void* param){
	
	std::string sourceStr;
	switch(source) {
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		sourceStr = "WindowSys";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		sourceStr = "App";
		break;
	case GL_DEBUG_SOURCE_API:
		sourceStr = "OpenGL";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		sourceStr = "ShaderCompiler";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		sourceStr = "3rdParty";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		sourceStr = "Other";
		break;
	default:
		sourceStr = "Unknown";
	}
	
	std::string typeStr;
	switch(type) {
	case GL_DEBUG_TYPE_ERROR:
		typeStr = "Error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		typeStr = "Deprecated";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		typeStr = "Undefined";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		typeStr = "Portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		typeStr = "Performance";
		break;
	case GL_DEBUG_TYPE_MARKER:
		typeStr = "Marker";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		typeStr = "PushGrp";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		typeStr = "PopGrp";
		break;
	case GL_DEBUG_TYPE_OTHER:
		typeStr = "Other";
		break;
	default:
		typeStr = "Unknown";
	}
	
	std::string sevStr;
	switch(severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		sevStr = "HIGH";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		sevStr = "MED";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		sevStr = "LOW";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		sevStr = "NOTIFY";
		break;
	default:
		sevStr = "UNK";
	}

    //if(source == GL_DEBUG_SOURCE_SHADER_COMPILER && type == GL_DEBUG_TYPE_OTHER) return;

    //printf("%s:%s[%s](%d): %s\n", sourceStr, typeStr, sevStr, id, message);
    LogError("%s:%s[%s](%d): %s\n", sourceStr.c_str(), typeStr.c_str(), sevStr.c_str(), id, message);
}
#endif

void OpenGLGraphicsDevice::Initialize(){
    auto CreateLineVAO = [&](unsigned int* vao, unsigned int* vbo, int vertexCount){
        #ifdef USE_VAO
        glGenVertexArrays(1, vao);
        glBindVertexArray(*vao);
        glCheckError();
        #endif
        
        glGenBuffers(1, vbo);
        glBindBuffer(GL_ARRAY_BUFFER, *vbo);
        //glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexCount * 3, NULL, GL_DYNAMIC_DRAW);
        glCheckError();
    
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
        glEnableVertexAttribArray(0);
        glCheckError();
    
        #ifdef USE_VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glCheckError();
        #endif
    };
    
    auto CreateWiredCubeVAO = [&](unsigned int& wiredCubeVAO, unsigned int& wiredCubeEBO, unsigned int& wiredCubeVBO){
        float vertex[] = {
            -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,   0.5f, 0.5f, -0.5f,   -0.5f, 0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,   0.5f, 0.5f,  0.5f,   -0.5f, 0.5f,  0.5f
        };
    
        unsigned int indices[] = {
            0, 1, 1, 2, 2, 3, 3, 0, 
            4, 5, 5, 6, 6, 7, 7, 4,
            0, 4, 1, 5, 2, 6, 3, 7
        };
    
        #ifdef USE_VAO
        glGenVertexArrays(1, &wiredCubeVAO);
        glBindVertexArray(wiredCubeVAO);
        glCheckError();
        #endif
        
    
        glGenBuffers(1, &wiredCubeEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wiredCubeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glCheckError();
    
        glGenBuffers(1, &wiredCubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, wiredCubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);
        glCheckError();
    
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
        glEnableVertexAttribArray(0);
        glCheckError();
    
        #ifdef USE_VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glCheckError();
        #endif
    };
    
    auto CreateTextQuadVAO = [&](unsigned int& textQuadVAO, unsigned int& textQuadVBO){
        #ifdef USE_VAO
        glGenVertexArrays(1, &textQuadVAO);
        glBindVertexArray(textQuadVAO);
        glCheckError();
        #endif
    
        glGenBuffers(1, &textQuadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 5, NULL, GL_DYNAMIC_DRAW);
        glCheckError();
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
        glCheckError();
    
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glCheckError();
    
        #ifdef USE_VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);   
        glCheckError();
        #endif
    };

    LogInfo("OpenGLGraphicsDevice::Initialize");

    glViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    LogInfo("OpenGLGraphicsDevice::Initialize2");
    //#if !defined(__EMSCRIPTEN__)
    ImGui_ImplOpenGL3_Init(
        OpenglHeader 
        //"#version 150"
    );
    //#endif

    #ifdef OPENGL_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(DebugCallback, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    #endif

    LogInfo("Opengl Version: %s", glGetString(GL_VERSION));
    LogInfo("GL_VENDOR: %s", glGetString(GL_VENDOR));
    LogInfo("GL_RENDERER: %s", glGetString(GL_RENDERER));
    LogInfo("GL_SHADING_LANGUAGE_VERSION: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glEnable(GL_DEPTH_TEST); 

    #ifndef USE_VAO
    glGenVertexArrays(1, &globalVAO);
	glBindVertexArray(globalVAO);
    glCheckError();
    #endif

    auto defaultSkybox = Cubemap::CreateFromFile(
        "Engine/Textures/Skybox/right.jpg",
        "Engine/Textures/Skybox/left.jpg",
        "Engine/Textures/Skybox/top.jpg",
        "Engine/Textures/Skybox/bottom.jpg",
        "Engine/Textures/Skybox/front.jpg",
        "Engine/Textures/Skybox/back.jpg"
    );
    AssetManager::Get().AddAsset("DefaultSkyboxCubemap", defaultSkybox);

    fullScreenQuad = Mesh::FullScreenQuad();
   
    /*gismoShader = SubShader::CreateFromFile("Engine/Shaders/Gizmos.glsl");
    Assert(gismoShader != nullptr);*/

    gismoMaterial = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Gizmos.glsl"));

    CreateLineVAO(&lineVAO, &lineVBO, 2);
    CreateLineVAO(&lineCommandsVAO, &lineCommandsVBO, MAX_LINES_VERTEX_DRAWCALL*2);
    CreateWiredCubeVAO(wiredCubeVAO, wiredCubeEBO, wiredCubeVBO);
    CreateTextQuadVAO(textQuadVAO, textQuadVBO);

    GLint maxLayers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);
    LogInfo("MaxArrayTextureLayers: %d", maxLayers);

    //#if OPENGL_DEBUG
    GLint maxVertexUniformComponents;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniformComponents);
    LogInfo("MaxVertexUniformComponents: %d", maxVertexUniformComponents);
    //glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniformComponents);
    //LogInfo("MaxVertexUniformComponentVectors: %d", maxVertexUniformComponents);

    GLint maxFragmentUniformComponents;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragmentUniformComponents);
    LogInfo("MaxFragmentUniformComponents: %d", maxFragmentUniformComponents);
    //glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxFragmentUniformComponents);
    //LogInfo("MaxFragmentUniformComponentVectors: %d", maxFragmentUniformComponents);
    //#endif

    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    LogInfo("MaxTextureSize: %d", maxTextureSize);

    GLint maxTextureUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    LogInfo("MaxTextureUnits: %d", maxTextureUnits);

    #if UseUniformBuffer
    glGenBuffers(1, &cameraDataBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraDataBuffer);
    glCheckError();

    glGenBuffers(1, &globalDataBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, globalDataBuffer);
    glCheckError();

    glGenBuffers(1, &renderPipelineDataBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, renderPipelineDataBuffer);
    glCheckError();
    
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glCheckError();
    #endif
}

void OpenGLGraphicsDevice::Shutdown(){
    fullScreenQuad = nullptr;
    //#if !defined(__EMSCRIPTEN__)
    ImGui_ImplOpenGL3_Shutdown();
    //#endif
}

void OpenGLGraphicsDevice::Begin(){
    stats.drawCalls = 0;
    stats.vertices = 0;
    stats.tris = 0;
    stats.shaderBinds = 0;
    stats.uniformSet = 0;
    stats.materialSubmitDatas = 0;
    begin = true;
    lastMat = nullptr;
    lastShader = nullptr;
}

void OpenGLGraphicsDevice::End(){
    begin = false;
}

void OpenGLGraphicsDevice::_Begin(){
    #ifndef USE_VAO
    glBindVertexArray(globalVAO);
    glCheckError();
    #endif
}

void OpenGLGraphicsDevice::_End(){
    #ifndef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
}

bool OpenGLGraphicsDevice::HasBegin(){
    return begin;
}

void OpenGLGraphicsDevice::BeginRenderToScreen(Vector4 clearColor){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Clean(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
}

void OpenGLGraphicsDevice::EndRenderToScreen(){
    Application::DrawImGui();
}

void OpenGLGraphicsDevice::Clean(float r, float g, float b, float a){
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
}

void OpenGLGraphicsDevice::SetCamera(Camera& inCamera){
    camera = inCamera;

    #if UseUniformBuffer
    struct CameraData{
        Matrix4 projection;
        Matrix4 view;
    };
    CameraData data = {camera.projection, camera.view};

    glBindBuffer(GL_UNIFORM_BUFFER, cameraDataBuffer);
    glCheckError();
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &data, GL_STATIC_DRAW); //GL_DYNAMIC_DRAW
    glCheckError();
    #endif
}

Camera OpenGLGraphicsDevice::GetCamera(){
    return camera;
}

void OpenGLGraphicsDevice::SetColorMask(Vector4 mask){
    glColorMask(mask.r, mask.g, mask.b, mask.a);
}   

void OpenGLGraphicsDevice::SetRenderMode(RenderMode mode){
    if(mode == RenderMode::SHADED) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if(mode == RenderMode::WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glCheckError();
}

void OpenGLGraphicsDevice::SetDepthMask(bool value){
    if(value){
        glDepthMask(GL_TRUE);
    } else {
        glDepthMask(GL_FALSE);
    }
    glCheckError();
}

void OpenGLGraphicsDevice::SetDepthTest(DepthTest depthTest){
    switch(depthTest){
      case DepthTest::DISABLE:
        glDisable(GL_DEPTH_TEST);
        break;
      case DepthTest::LESS:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);  
        break;
      case DepthTest::LESS_EQUAL:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);  
        break;
      case DepthTest::EQUAL:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_EQUAL);  
        break;
      case DepthTest::GREATER:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GREATER);  
        break;
      case DepthTest::GREATER_EQUAL:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);  
        break;
      case DepthTest::DIFFERENT:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_NOTEQUAL);  
        break;
      case DepthTest::ALWAYS:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);  
        break;
      case DepthTest::NEVER:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_NEVER);  
        break;
    }
}

void OpenGLGraphicsDevice::SetCullFace(CullFace cullFace){
    switch(cullFace){
      case CullFace::BACK:
        glEnable(GL_CULL_FACE); 
        glCullFace(GL_BACK);
        break;

      case CullFace::FRONT:
        glEnable(GL_CULL_FACE); 
        glCullFace(GL_FRONT);
        break;

      case CullFace::FRONT_AND_BACK:
        glEnable(GL_CULL_FACE); 
        glCullFace(GL_FRONT_AND_BACK);
        break;

      case CullFace::NONE:
        glDisable(GL_CULL_FACE);
        break;
    }
}

void OpenGLGraphicsDevice::SetBlend(bool b){
    if(b){
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    glCheckError();
}

int OpenGLGraphicsDevice::BlendModeToGL(BlendMode blendMode){
    if(blendMode == BlendMode::ZERO) return GL_ZERO;
    if(blendMode == BlendMode::ONE) return GL_ONE;
    if(blendMode == BlendMode::SRC_COLOR) return GL_SRC_COLOR;
    if(blendMode == BlendMode::ONE_MINUS_SRC_COLOR) return GL_ONE_MINUS_SRC_COLOR;
    if(blendMode == BlendMode::DST_COLOR) return GL_DST_COLOR;
    if(blendMode == BlendMode::ONE_MINUS_DST_COLOR) return GL_ONE_MINUS_DST_COLOR;
    if(blendMode == BlendMode::SRC_ALPHA) return GL_SRC_ALPHA;
    if(blendMode == BlendMode::ONE_MINUS_SRC_ALPHA) return GL_ONE_MINUS_SRC_ALPHA;
    if(blendMode == BlendMode::DST_ALPHA) return GL_DST_ALPHA;
    if(blendMode == BlendMode::ONE_MINUS_DST_ALPHA) return GL_ONE_MINUS_DST_ALPHA;
    if(blendMode == BlendMode::CONSTANT_COLOR) return GL_CONSTANT_COLOR;
    if(blendMode == BlendMode::ONE_MINUS_CONSTANT_COLOR) return GL_ONE_MINUS_CONSTANT_COLOR;
    if(blendMode == BlendMode::CONSTANT_ALPHA) return GL_CONSTANT_ALPHA;
    if(blendMode == BlendMode::ONE_MINUS_CONSTANT_ALPHA) return GL_ONE_MINUS_CONSTANT_ALPHA;

    Assert(false);
    return 0;
}

void OpenGLGraphicsDevice::SetBlendFunc(BlendMode sfactor, BlendMode dfactor){
    glBlendFunc(BlendModeToGL(sfactor), BlendModeToGL(dfactor));
    glCheckError();
}

int OpenGLGraphicsDevice::SubShaderGetLocation(SubShader& shader, const char* name){
    //return shader.glData.uniforms[name];
    if(shader.glData.uniforms.count(name)) return shader.glData.uniforms[name];
    
    GLint location = glGetUniformLocation(shader.glData.id, name);
    glCheckError();
    
    shader.glData.uniforms[name] = location;
    return location;
}

void OpenGLGraphicsDevice::SubShaderSetFloat(SubShader& shader, const char* name, float value){
    Graphics::GetStats().uniformSet += 1;
    glUniform1f(SubShaderGetLocation(shader, name), value);
    glCheckError();
    /*glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });*/
}

void OpenGLGraphicsDevice::SubShaderSetFloat(SubShader& shader, const char* name, float* value, int count){
    stats.uniformSet += 1;
    glUniform1fv(SubShaderGetLocation(shader, name), (GLsizei)count, (GLfloat*)value);
    glCheckError();
}

void OpenGLGraphicsDevice::SubShaderSetInt(SubShader& shader, const char* name, int value){
    stats.uniformSet += 1;
    glUniform1i(SubShaderGetLocation(shader, name), value);
    glCheckError();
    /*glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });*/
}

void OpenGLGraphicsDevice::SubShaderSetVector2(SubShader& shader, const char* name, Vector2 value){
    Graphics::GetStats().uniformSet += 1;
    glUniform2f(SubShaderGetLocation(shader, name), value.x, value.y);
    glCheckError();
}

void OpenGLGraphicsDevice::SubShaderSetVector3(SubShader& shader, const char* name, Vector3 value){
    Graphics::GetStats().uniformSet += 1;
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform3f(SubShaderGetLocation(shader, name), value.x, value.y, value.z);
    glCheckError();
    /*glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });*/
}

void OpenGLGraphicsDevice::SubShaderSetVector4(SubShader& shader, const char* name, Vector4 value){
    Graphics::GetStats().uniformSet += 1;
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform4f(SubShaderGetLocation(shader, name), value.x, value.y, value.z, value.w);
    glCheckError();
}

void OpenGLGraphicsDevice::SubShaderSetVector4(SubShader& shader, const char* name, Vector4* value, int count){
    Graphics::GetStats().uniformSet += 1;
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniform4fv(SubShaderGetLocation(shader, name), (GLsizei)count, (GLfloat*)value);
    glCheckError();
}

void OpenGLGraphicsDevice::SubShaderSetMatrix4(SubShader& shader, const char* name, Matrix4 value){
    Graphics::GetStats().uniformSet += 1;
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniformMatrix4fv(SubShaderGetLocation(shader, name), 1, GL_FALSE, glm::value_ptr(static_cast<glm::mat4>(value)));
    glCheckError();
    /*glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s", name, path.c_str()); 
    });*/
}

void OpenGLGraphicsDevice::SubShaderSetMatrix4(SubShader& shader, const char* name, std::vector<Matrix4>& value){
    Graphics::GetStats().uniformSet += 1;
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    glUniformMatrix4fv(SubShaderGetLocation(shader, name), (GLsizei)value.size(), GL_FALSE, glm::value_ptr(value[0]));
    //Set(GetLocation(name), &value[0], (unsigned int)value.size());
    glCheckError();
    /*glCheckError2([&](){ 
        LogError("UniformName: %s ShaderPath: %s Cout: %zd", name, path.c_str(), value.size()); 
    });*/
}

void OpenGLGraphicsDevice::SubShaderSetMatrix4(SubShader& shader, const char* name, Matrix4* value, int count){
    Graphics::GetStats().uniformSet += 1;
    //if(curBindShaderRenderId != rendererId) Bind(*this);
    glUniformMatrix4fv(SubShaderGetLocation(shader, name), (GLsizei)count, GL_FALSE, (GLfloat*)value);
    glCheckError();
}

void OpenGLGraphicsDevice::Texture2DBind(Texture2D& tex, int index){
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, tex.glData.id);
    glCheckError();
}

void OpenGLGraphicsDevice::Texture2DArrayBind(Texture2DArray& tex, int index){
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.glData.id);
    glCheckError();
}

void OpenGLGraphicsDevice::CubemapBind(Cubemap& cubemap, int index){
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.glData.id);
    glCheckError();
}

void OpenGLGraphicsDevice::SubShaderSetTexture2D(SubShader& shader, const char* name, Texture2D& value, int index){
    Graphics::GetStats().uniformSet += 1;
    //glActiveTexture(GL_TEXTURE0 + index); glCheckError();
    Texture2DBind(value, index);
    SubShaderSetInt(shader, name, index);
}

void OpenGLGraphicsDevice::SubShaderSetTexture2DArray(SubShader& shader, const char* name, Texture2DArray& value, int index){
    Graphics::GetStats().uniformSet += 1;
    //glActiveTexture(GL_TEXTURE0 + index); glCheckError();
    Texture2DArrayBind(value, index);
    SubShaderSetInt(shader, name, index);
}

void OpenGLGraphicsDevice::SubShaderSetCubemap(SubShader& shader, const char* name, Cubemap& value, int index){
    Graphics::GetStats().uniformSet += 1;
    //glActiveTexture(GL_TEXTURE0 + index); glCheckError();
    CubemapBind(value, index);
    SubShaderSetInt(shader, name, index);
}

void OpenGLGraphicsDevice::SubShaderSetFramebuffer(SubShader& shader, const char* name, Framebuffer& framebuffer, int index, int colorAttachmentIndex){
    stats.uniformSet += 1;

    Assert(framebuffer.Specification().type != FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE);
    //Assert(framebuffer.specification().sample <= 1);

    glActiveTexture(GL_TEXTURE0 + index);
    glCheckError();

    unsigned int target = GL_TEXTURE_2D;
    if(framebuffer.Specification().type == FramebufferAttachmentType::TEXTURE_2D_ARRAY){
        target = GL_TEXTURE_2D_ARRAY;
    }

    if(colorAttachmentIndex == -1){
        //glBindTexture(target, framebuffer.DepthAttachmentId());
        glBindTexture(target, framebuffer.glData.depthAttachment);
        glCheckError();
    } else {
        //glBindTexture(target, framebuffer.ColorAttachmentId(colorAttachmentIndex));
        glBindTexture(target, framebuffer.glData.colorAttachments[colorAttachmentIndex]);
        glCheckError();
    }

    //glCheckError();
    SubShaderSetInt(shader, name, index);
}

/*void SubShaderSetUniforBuffer(GLSubShaderData& shader, const char* name, UniformBuffer& buffer, int index){
    Graphics::GetStats().uniformSet += 1;
    //if(curBindShaderRenderId != rendererId) Bind(*this);

    UniformBuffer::Bind(buffer, index);
    unsigned int bufferIndex = glGetUniformBlockIndex(shader.id, name);   
    glUniformBlockBinding(shader.id, bufferIndex, index);
    glCheckError();
}*/

void OpenGLGraphicsDevice::BindMaterial(Material& mat){
    auto ContainUniformName = [&](SubShader shader, const std::string& name){ 
        return std::find(shader.glData._uniforms.begin(), shader.glData._uniforms.end(), name) != shader.glData._uniforms.end(); 
    };

    auto ApplyUniformTo = [&](Material& material, SubShader& shader, std::unordered_map<std::string, MaterialMap>& maps){
        for(auto& i: maps){
            MaterialMap& map = i.second;

            #if UseUniformBuffer
            if(material.glData.mainUniformData != nullptr && material.glData.mainBufferDef.members.count(i.first)){
                UniformBufferDef::Member m = material.glData.mainBufferDef.members[i.first];

                if(map.type == MaterialMap::Type::Int){
                    Assert(m.size >= sizeof(int));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.valueInt, sizeof(int));
                } else if(map.type == MaterialMap::Type::Float){
                    Assert(m.size >= sizeof(float));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.valueFloat, sizeof(float));
                } else if(map.type == MaterialMap::Type::Vector2){
                    Assert(m.size >= sizeof(Vector2));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector2));
                } else if(map.type == MaterialMap::Type::Vector3){
                    Assert(m.size >= sizeof(Vector3));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector3));
                } else if(map.type == MaterialMap::Type::Vector4){
                    Assert(m.size >= sizeof(Vector4));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector4));
                } else if(map.type == MaterialMap::Type::Matrix4){
                    Assert(m.size >= sizeof(Matrix4));
                    memcpy((char*)material.glData.mainUniformData + m.pos, &map.matrix, sizeof(Matrix4));
                } else if(map.type == MaterialMap::Type::FloatList){
                    Assert(m.size >= sizeof(float) * map.listCount);
                    memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<float*>(map.list), sizeof(float) * map.listCount);
                } else if(map.type == MaterialMap::Type::Vector4List){
                    Assert(m.size >= sizeof(Vector4) * map.listCount);
                    memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<Vector4*>(map.list), sizeof(Vector4) * map.listCount);
                } else if(map.type == MaterialMap::Type::Matrix4List){
                    Assert(m.size >= sizeof(Matrix4) * map.listCount);
                    memcpy((char*)material.glData.mainUniformData + m.pos, static_cast<Matrix4*>(map.list), sizeof(Matrix4) * map.listCount);
                } else {
                    Assert(false && "Type Not Supported in A UnifomBuffer");
                }
                continue;
            }
            #endif
            
            if(ContainUniformName(shader, i.first) == false) continue;

            if(map.type == MaterialMap::Type::Int){
                SubShaderSetInt(shader, i.first.c_str(), map.valueInt);
            }
            if(map.type == MaterialMap::Type::Float){
                SubShaderSetFloat(shader, i.first.c_str(), map.valueFloat);
            }
            if(map.type == MaterialMap::Type::Vector2){
                SubShaderSetVector2(shader, i.first.c_str(), Vector2(map.vec.vector.x, map.vec.vector.y));
            }
            if(map.type == MaterialMap::Type::Vector3){
                SubShaderSetVector3(shader, i.first.c_str(), Vector3(map.vec.vector.x, map.vec.vector.y, map.vec.vector.z));
            }
            if(map.type == MaterialMap::Type::Vector4){
                SubShaderSetVector4(shader, i.first.c_str(), map.vec.vector);
            }
            if(map.type == MaterialMap::Type::Matrix4){
                SubShaderSetMatrix4(shader, i.first.c_str(), i.second.matrix);
            }
            if(map.type == MaterialMap::Type::Texture){
                Assert(i.second.texture != nullptr);
                SubShaderSetTexture2D(shader, i.first.c_str(), *i.second.texture, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::TextureArray){
                SubShaderSetTexture2DArray(shader, i.first.c_str(), *i.second.textureArray, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::Framebuffer){
                SubShaderSetFramebuffer(shader, i.first.c_str(), *i.second.framebuffer, material.currentTextureSlot, map.framebufferAttachment);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::Cubemap){
                SubShaderSetCubemap(shader, i.first.c_str(), *i.second.cubemap, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::FloatList){
                SubShaderSetFloat(shader, i.first.c_str(), static_cast<float*>(map.list), map.listCount);
            }
            if(map.type == MaterialMap::Type::Vector4List){
                SubShaderSetVector4(shader, i.first.c_str(), static_cast<Vector4*>(map.list), map.listCount);
            }
            if(map.type == MaterialMap::Type::Matrix4List){
                SubShaderSetMatrix4(shader, i.first.c_str(), static_cast<Matrix4*>(map.list), map.listCount);
            }
        }
    };

    auto SubmitGraphicDatas = [&](Material& material){
        stats.materialSubmitDatas += 1;
        material.currentTextureSlot = 0;
        material.UpdateCurrentShader();

        Assert(material.GetShader() != nullptr);
        if(material.GetShader() == nullptr) return;

        /*SetColorMask(mat.currentShader->pipeline.colorMask);
        SetCullFace(mat.currentShader->GetCullFace());
        SetDepthTest(mat.currentShader->GetDepthTest());
        SetDepthMask(mat.currentShader->IsDepthMask());
        if(mat.currentShader->IsBlend()){
            SetBlend(true);
            SetBlendFunc(mat.currentShader->GetSrcBlend(), mat.currentShader->GetDstBlend());
        } else {
            SetBlend(false);
        }*/

        SubShaderBind(*material.currentShader);
        ApplyUniformTo(material, *material.currentShader, material.maps);
        ApplyUniformTo(material, *material.currentShader, Material::globalMaps);
        Assert(material.currentTextureSlot < 32);
    };

    Assert(mat.currentShader != nullptr && "Shader is not vali!");
    Assert(mat.GetShader()->IsComplete() == true && "Shader is not vali!");

    if(&mat != lastMat || mat.isDirty == true){
        SubmitGraphicDatas(mat);
        mat.isDirty = false;
        #if UseUniformBuffer
        glBindBuffer(GL_UNIFORM_BUFFER, mat.glData.mainBuffer);
        glCheckError();
        glBufferData(GL_UNIFORM_BUFFER, mat.glData.mainBufferDef.size, mat.glData.mainUniformData, GL_STATIC_DRAW); //GL_DYNAMIC_DRAW
        glCheckError();
        #endif
    }
    lastMat = &mat;
    
    if(mat.currentShader.get() != lastShader){
        #if UseUniformBuffer
        unsigned int index = glGetUniformBlockIndex(mat.currentShader->glData.id, "CamDraw");   
        if(index != GL_INVALID_INDEX){
            glBindBuffer(GL_UNIFORM_BUFFER, cameraDataBuffer);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraDataBuffer);
            glCheckError(); 
            glUniformBlockBinding(mat.currentShader->glData.id, index, 0);
            glCheckError(); 
        } else {
            SubShaderSetMatrix4(*mat.currentShader, "projection", camera.projection); //mat.currentShader->SetMatrix4("projection", camera.projection);
            SubShaderSetMatrix4(*mat.currentShader, "view", camera.view); //mat.currentShader->SetMatrix4("view", camera.view);
        }
        unsigned int index2 = glGetUniformBlockIndex(mat.currentShader->glData.id, "Main");  
        if(index2 != GL_INVALID_INDEX){
            glBindBuffer(GL_UNIFORM_BUFFER, mat.glData.mainBuffer);
            glBindBufferBase(GL_UNIFORM_BUFFER, 1, mat.glData.mainBuffer);
            glCheckError(); 
            glUniformBlockBinding(mat.currentShader->glData.id, index2, 1);
            glCheckError(); 
        }       
        #else
        //SubShaderBind(*mat.currentShader);
        SubShaderSetMatrix4(*mat.currentShader, "projection", camera.projection); //mat.currentShader->SetMatrix4("projection", camera.projection);
        SubShaderSetMatrix4(*mat.currentShader, "view", camera.view); //mat.currentShader->SetMatrix4("view", camera.view);
        #endif
    }
    lastShader = mat.currentShader.get();
}  

void OpenGLGraphicsDevice::DrawMesh(Mesh& mesh, Matrix4 modelMatrix){
    if(MeshIsValid(mesh) == false){
        #ifdef GRAPHIC_LOG_ERROR
        LogError("DrawMesh::InvalidMesh");
        #endif
        return;
    }

    Assert(MeshIsValid(mesh) && "Mesh is not vali!");

    SubShaderSetMatrix4(*lastShader, "model", modelMatrix);
    
    stats.drawCalls += 1;
    stats.vertices += mesh.vertexCount;
    stats.tris += mesh.indiceCount;
    
    #ifdef USE_VAO
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
    #else
    mesh.Bind();
    #endif

    if(mesh.glData.ebo != 0){
        glDrawElements(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0);
        glCheckError();
    } else {
        glDrawArrays(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount);
        glCheckError();
    }
}

void OpenGLGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Matrix4 modelMatrix, Matrix4* animMatrixs, int count){
    if(MeshIsValid(mesh) == false){
        #ifdef GRAPHIC_LOG_ERROR
        LogError("DrawMesh::InvalidMesh");
        #endif
        return;
    }

    Assert(MeshIsValid(mesh) && "Mesh is not vali!");
    
    SubShaderSetMatrix4(*lastShader, "animated", animMatrixs, count);
    SubShaderSetMatrix4(*lastShader, "model", modelMatrix);
    
    stats.drawCalls += 1;
    stats.vertices += mesh.vertexCount;
    stats.tris += mesh.indiceCount;
    
    #ifdef USE_VAO
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
    #else
    mesh.Bind();
    #endif

    if(mesh.glData.ebo != 0){
        glDrawElements(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0);
        glCheckError();
    } else {
        glDrawArrays(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount);
        glCheckError();
    }
}

void OpenGLGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Matrix4* modelMatrixs, int count){
    Assert(MeshIsValid(mesh) && "Mesh is not vali!");

    mesh.SubmitInstancingCustomModelMatrixs(modelMatrixs, count);

    stats.drawCalls += 1;
    stats.vertices += mesh.vertexCount * count;
    stats.tris += mesh.indiceCount * count;

    #ifdef USE_VAO
    glBindVertexArray(mesh.glData.vao);
    #else
    mesh.Bind();
    #endif

    if(mesh.glData.ebo != 0){
        glDrawElementsInstanced(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0, count);
        glCheckError();
    } else {
        glDrawArraysInstanced(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount, count);
        glCheckError();
    }

    //glBindVertexArray(0);
    //glCheckError();
}

void OpenGLGraphicsDevice::DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix){
    BindMaterial(mat);
    DrawMesh(mesh, modelMatrix);
}

void OpenGLGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 modelMatrix, Matrix4* animMatrixs, int count){
    BindMaterial(mat);
    DrawMeshSkinned(mesh, modelMatrix, animMatrixs, count);
}

void OpenGLGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4* modelMatrixs, int count){
    BindMaterial(mat);
    DrawMeshInstancing(mesh, modelMatrixs, count);
}

void OpenGLGraphicsDevice::DrawModel(Model& model, Matrix4 modelMatrix){
    int index = 0;
    for(auto i: model.renderTargets){
        Ref<Material> targetMaterial = model.materials[i.materialIndex];
        Ref<Mesh> targetMesh = model.meshs[i.meshIndex];
        Matrix4 targetMatrix =  modelMatrix * model.skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
        BindMaterial(*targetMaterial);
        DrawMesh(*targetMesh, targetMatrix);
    }
}

void OpenGLGraphicsDevice::AddDrawLineCommand(Vector3 start, Vector3 end){
    lineCommandsData.push_back(start.x);
    lineCommandsData.push_back(start.y);
    lineCommandsData.push_back(start.z);

    lineCommandsData.push_back(end.x);
    lineCommandsData.push_back(end.y);
    lineCommandsData.push_back(end.z);
}

void OpenGLGraphicsDevice::DrawLinesComamnd(Vector3 color, int lineWidth){
    if(lineWidth > 1) lineWidth = 1;

    //drawCalls += 1;
    stats.vertices += lineCommandsData.size()/3;
    stats.tris += 0;

    BindMaterial(*gismoMaterial);
    Assert(lastShader != nullptr);
    SubShaderSetVector3(*lastShader, "color", color);
    SubShaderSetMatrix4(*lastShader, "model", Matrix4Identity);

    /*SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);*/

	glLineWidth(lineWidth);
    glCheckError();

    #ifdef USE_VAO
	glBindVertexArray(lineCommandsVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineCommandsVBO);
    glCheckError();
    #else
    glBindBuffer(GL_ARRAY_BUFFER, lineCommandsVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
    glCheckError();
    #endif
    
    /*glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * lineCommandsData.size(), &lineCommandsData[0]);
	glCheckError();
    glDrawArrays(GL_LINES, 0, lineCommandsData.size()/3);
    glCheckError();*/

    for(int i = 0; i < lineCommandsData.size(); i += 2 * MAX_LINES_VERTEX_DRAWCALL * 3){
        stats.drawCalls += 1;
        int batchVertexCount = std::min<int>(lineCommandsData.size() - i, 2 * MAX_LINES_VERTEX_DRAWCALL * 3);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * batchVertexCount, &lineCommandsData[i]);
	    glCheckError();
        glDrawArrays(GL_LINES, 0, batchVertexCount/3);
        glCheckError();
    }

    lineCommandsData.clear();
}

void OpenGLGraphicsDevice::DrawLine(Vector3 start, Vector3 end, Vector3 color, int width){
    stats.drawCalls += 1;
    stats.vertices += 2;
    stats.tris += 0;

    BindMaterial(*gismoMaterial);
    Assert(lastShader != nullptr);
    SubShaderSetVector3(*lastShader, "color", color);
    SubShaderSetMatrix4(*lastShader, "model", Matrix4Identity);

    /*SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);*/

	glLineWidth(width);

    float line[6] = {start.x, start.y, start.z, end.x, end.y, end.z};

    #ifdef USE_VAO
	glBindVertexArray(lineVAO);
    #else
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
    glCheckError();
    #endif

	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line), line);
	glDrawArrays(GL_LINES, 0, 2);
    glCheckError();

	//glBindVertexArray(0);
    //glCheckError();
}

void OpenGLGraphicsDevice::DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int width){
    stats.drawCalls += 1;
    stats.vertices += 2;
    stats.tris += 0;

    start = Vector3(model * Vector4(start.x, start.y, start.z, 1));
    end = Vector3(model * Vector4(end.x, end.y, end.z, 1));

    BindMaterial(*gismoMaterial);
    Assert(lastShader != nullptr);
    SubShaderSetVector3(*lastShader, "color", color);
    SubShaderSetMatrix4(*lastShader, "model", Matrix4Identity);

    /*SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity); //gismoShader->SetMatrix4("model", model);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);*/

	glLineWidth(width);

    float line[6] = {start.x, start.y, start.z, end.x, end.y, end.z};

    #ifdef USE_VAO
	glBindVertexArray(lineVAO);
    #else
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
    glCheckError();
    #endif

	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line), line);
	glDrawArrays(GL_LINES, 0, 2);
    glCheckError();
	
    //glBindVertexArray(0);
    //glCheckError();
}

void OpenGLGraphicsDevice::DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth){
    stats.drawCalls += 1;
    stats.vertices += 8;
    stats.tris += 24;

    BindMaterial(*gismoMaterial);
    Assert(lastShader != nullptr);
    SubShaderSetVector3(*lastShader, "color", color);
    SubShaderSetMatrix4(*lastShader, "model", modelMatrix);

    /*SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", modelMatrix);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);*/

    #ifdef USE_VAO
    glBindVertexArray(wiredCubeVAO);
    #else
    glBindBuffer(GL_ARRAY_BUFFER, wiredCubeVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wiredCubeEBO);
    glCheckError();
    #endif
    
    glLineWidth(lineWidth);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    glCheckError();

    //glBindVertexArray(0);
    //glCheckError();
}

void OpenGLGraphicsDevice::DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& mat, int pass){
    Assert(false && "Deprecated");

    /*Assert(src != nullptr);
    Assert(src->IsValid() == true);

    if(dst == nullptr){
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glCheckError();
    } else {
        BeginFramebuffer(*dst, 0);
        Graphics::SetViewport(0, 0, dst->Width(), dst->Height());
    }
    
    mat.SetTexture("mainTex", src, pass);
    //BindMaterial(mat);
    Graphics::Clean(0, 0, 0, 1);
    DrawMesh(*fullScreenQuad, mat, Matrix4Identity);*/
}

void OpenGLGraphicsDevice::DrawQuadPostProcessing(Framebuffer* dst, Material& mat, int pass){
    Assert(false && "Deprecated");
    /*if(dst == nullptr){
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glCheckError();
    } else {
        BeginFramebuffer(*dst, 0);
        Graphics::SetViewport(0, 0, dst->Width(), dst->Height());
    }

    //BindMaterial(mat);
    Graphics::Clean(0, 0, 0, 1);
    DrawMesh(*fullScreenQuad, mat, Matrix4Identity);*/
}

void OpenGLGraphicsDevice::SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){
    glViewport(x, y, w, h);
}

void OpenGLGraphicsDevice::GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h){
    GLint value[4];
    glGetIntegerv(GL_VIEWPORT, value);
    *x = value[0]; 
    *y = value[1];
    *w = value[2]; 
    *h = value[3];
}

bool OpenGLGraphicsDevice::MeshCreateOrSubmit(
    Mesh& mesh,
    std::vector<unsigned int>* indices,
    std::vector<Vector3>* vertices,
    std::vector<Vector3>* uv,
    std::vector<Vector3>* normals,
    std::vector<Vector4>* colors,
    std::vector<Vector3>* tangents,
    std::vector<Vector4>* weights,
    std::vector<IVector4>* influences
){
    //Assert(isReadable == true && "Only can Update isReadable Mesh");

    //Destroy();

    MeshDestroy(mesh);

    #ifdef USE_VAO
    if(mesh.glData.vao == 0){
        glGenVertexArrays(1, &mesh.glData.vao);
        glBindVertexArray(mesh.glData.vao);
        glCheckError();
    } else {
        glBindVertexArray(mesh.glData.vao);
    }
    #endif
    
    if(mesh.glData.vertexVbo == 0){
        glGenBuffers(1, &mesh.glData.vertexVbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * vertices->size(), &(*vertices)[0], GL_STATIC_DRAW ); //GL_STATIC_DRAW
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
        glCheckError();
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * vertices->size(), &(*vertices)[0], GL_DYNAMIC_DRAW); //GL_STATIC_DRAW
    }
    mesh.vertexCount = vertices->size();

    if(indices != nullptr){
    if(indices->size() > 0){
        if(mesh.glData.ebo == 0){
            glGenBuffers(1, &mesh.glData.ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.glData.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size() * sizeof(unsigned int), &(*indices)[0], GL_STATIC_DRAW );
            glCheckError();
        } else {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.glData.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size() * sizeof(unsigned int), &(*indices)[0], GL_DYNAMIC_DRAW);
        }
    }
    mesh.indiceCount = indices->size();
    }

    if(uv != nullptr && uv->size() == vertices->size()){
        if(mesh.glData.uvVbo == 0){
            glGenBuffers(1, &mesh.glData.uvVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.uvVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * uv->size(), &(*uv)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.uvVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * uv->size(), &(*uv)[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    if(normals != nullptr && normals->size() == vertices->size()){
        if(mesh.glData.normalVbo == 0){
            glGenBuffers(1, &mesh.glData.normalVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.normalVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * normals->size(), &(*normals)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.normalVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * normals->size(), &(*normals)[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    if(colors != nullptr && colors->size() == vertices->size()){
        if(mesh.glData.colorVbo == 0){
            glGenBuffers(1, &mesh.glData.colorVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.colorVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * colors->size(), &(*colors)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.colorVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * colors->size(), &(*colors)[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    if(tangents != nullptr && tangents->size() == vertices->size()){
        if(mesh.glData.tangentVbo == 0){
            glGenBuffers(1, &mesh.glData.tangentVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.tangentVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * tangents->size(), &(*tangents)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.tangentVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * tangents->size(), &(*tangents)[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    if(influences != nullptr && influences->empty() == false) Assert(influences->size() == vertices->size());
    if(influences != nullptr && influences->size() == vertices->size()){
        if(mesh.glData.jointVbo == 0){
            glGenBuffers(1, &mesh.glData.jointVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.jointVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(IVector4) * influences->size(), &(*influences)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(5);
            glVertexAttribIPointer(5, 4, GL_INT, sizeof(IVector4), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.jointVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(IVector4) * influences->size(), &(*influences)[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    if(weights != nullptr && weights->empty() == false) Assert(weights->size() == vertices->size());
    if(weights != nullptr && weights->size() == vertices->size()){
        if(mesh.glData.weightsVbo == 0){
            glGenBuffers(1, &mesh.glData.weightsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.weightsVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * weights->size(), &(*weights)[0], GL_STATIC_DRAW );
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.weightsVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * weights->size(), &(*weights)[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif

    return true;
}

void OpenGLGraphicsDevice::MeshSubmitInstancingModelMatrixs(Mesh& mesh){
    #ifdef USE_VAO
    Assert(mesh.glData.vao != 0);
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
    #endif

    if(mesh.instancingModelMatrixs.empty() == false){
        if(mesh.glData.instancingModelMatrixsVbo == 0){
            glGenBuffers(1, &mesh.glData.instancingModelMatrixsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo); 
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * mesh.instancingModelMatrixs.size(), &mesh.instancingModelMatrixs[0], GL_DYNAMIC_DRAW); //GL_STREAM_DRAW GL_DYNAMIC_DRAW
            glCheckError();

            std::size_t vec4Size = sizeof(glm::vec4);
            glEnableVertexAttribArray(10); 
            glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
            glEnableVertexAttribArray(11); 
            glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
            glEnableVertexAttribArray(12); 
            glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
            glEnableVertexAttribArray(13); 
            glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
            glCheckError();

            glVertexAttribDivisor(10, 1);
            glVertexAttribDivisor(11, 1);
            glVertexAttribDivisor(12, 1);
            glVertexAttribDivisor(13, 1);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo);
            //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Matrix4) * instancingModelMatrixs.size(), &instancingModelMatrixs[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * mesh.instancingModelMatrixs.size(), &mesh.instancingModelMatrixs[0], GL_DYNAMIC_DRAW); //GL_STREAM_DRAW
            glCheckError();
        }
    }

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
}

void OpenGLGraphicsDevice::MeshSubmitInstancingCustomModelMatrixs(Mesh& mesh, Matrix4* modelMatrixs, int count){
    #ifdef USE_VAO
    Assert(mesh.glData.vao != 0);
    glBindVertexArray(mesh.glData.vao);
    glCheckError();
    #endif

    if(count > 0){
        if(mesh.glData.instancingModelMatrixsVbo == 0){
            glGenBuffers(1, &mesh.glData.instancingModelMatrixsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * count, modelMatrixs, GL_DYNAMIC_DRAW); //GL_STREAM_DRAW
            glCheckError();

            std::size_t vec4Size = sizeof(glm::vec4);
            glEnableVertexAttribArray(10); 
            glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
            glEnableVertexAttribArray(11); 
            glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
            glEnableVertexAttribArray(12); 
            glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
            glEnableVertexAttribArray(13); 
            glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
            glCheckError();

            glVertexAttribDivisor(10, 1);
            glVertexAttribDivisor(11, 1);
            glVertexAttribDivisor(12, 1);
            glVertexAttribDivisor(13, 1);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, mesh.glData.instancingModelMatrixsVbo);
            //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Matrix4) * instancingModelMatrixs.size(), &instancingModelMatrixs[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * count, modelMatrixs, GL_DYNAMIC_DRAW); //GL_STREAM_DRAW
            glCheckError();
        }
    }

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
}

void OpenGLGraphicsDevice::MeshDestroy(Mesh& mesh){
    if(MeshIsValid(mesh) == false) return;

    if(mesh.glData.vertexVbo != 0) glDeleteBuffers(1, &mesh.glData.vertexVbo);
    if(mesh.glData.normalVbo != 0) glDeleteBuffers(1, &mesh.glData.normalVbo);
    if(mesh.glData.uvVbo != 0) glDeleteBuffers(1, &mesh.glData.uvVbo);
    if(mesh.glData.colorVbo != 0) glDeleteBuffers(1, &mesh.glData.colorVbo);
    if(mesh.glData.tangentVbo != 0) glDeleteBuffers(1, &mesh.glData.tangentVbo);
    if(mesh.glData.instancingModelMatrixsVbo != 0) glDeleteBuffers(1, &mesh.glData.instancingModelMatrixsVbo);
    if(mesh.glData.jointVbo != 0) glDeleteBuffers(1, &mesh.glData.jointVbo);
    if(mesh.glData.weightsVbo != 0) glDeleteBuffers(1, &mesh.glData.weightsVbo);

    if(mesh.glData.ebo != 0) glDeleteBuffers(1, &mesh.glData.ebo);

    #ifdef USE_VAO
    if(mesh.glData.vao != 0) glDeleteVertexArrays(1, &mesh.glData.vao);
    #endif

    mesh.vertexCount = 0;
    mesh.indiceCount = 0;
    mesh.glData.vertexVbo = 0;
    mesh.glData.normalVbo = 0;
    mesh.glData.colorVbo = 0;
    mesh.glData.tangentVbo = 0;
    mesh.glData.instancingModelMatrixsVbo = 0;
    mesh.glData.jointVbo = 0;
    mesh.glData.weightsVbo = 0;
    mesh.glData.uvVbo = 0;
    mesh.glData.ebo = 0;

    #ifdef USE_VAO
    mesh.glData.vao = 0;
    #endif

    glCheckError();
}

bool OpenGLGraphicsDevice::MeshIsValid(Mesh& mesh){
    #ifdef USE_VAO
    return mesh.glData.vao != 0;
    #else
    return mesh.glData.vertexVbo != 0;
    #endif
}

int InternalFormatLookup[] = {
    GL_NONE, GL_RGB, GL_RGBA8, GL_RGB16F, GL_RGBA16F, GL_RGB32F, GL_RGBA32F, GL_R32I, GL_DEPTH24_STENCIL8, GL_DEPTH_COMPONENT
};

int FormatLookup[] = {
    GL_NONE, GL_RGB, GL_RGBA, GL_RGB, GL_RGBA, GL_RGB, GL_RGBA, GL_RED_INTEGER, GL_DEPTH_STENCIL, GL_DEPTH_COMPONENT
};

bool IsDepthTypeFormat(FramebufferTextureFormat format){
    if(format == FramebufferTextureFormat::DEPTH4STENCIL8) return true;
    if(format == FramebufferTextureFormat::DEPTH_COMPONENT) return true;
    return false;
}

void OpenGLGraphicsDevice::BeginFramebuffer(Framebuffer& frambuffer, Vector4 clearColor, int layer){
    Assert(frambuffer.glData.renderId > 0);
    glBindFramebuffer(GL_FRAMEBUFFER, frambuffer.glData.renderId);
    glCheckError();

    if(frambuffer.specification.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY){
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frambuffer.glData.depthAttachment, 0, layer);
        glCheckError();
    }

    // Set Attachment glFramebufferTexture2D
    glCheckError();
    //glViewport(0, 0, frambuffer.specification.width, frambuffer.specification.height);

    Clean(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
}

void OpenGLGraphicsDevice::EndFramebuffer(){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
}

void OpenGLGraphicsDevice::BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass){
    Assert(src != nullptr);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, src->glData.renderId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst == nullptr ? 0 : dst->glData.renderId);
    glCheckError();

    if(srcPass < 0){
        glBlitFramebuffer(0, 0, src->Width(), src->Height(), 0, 0, src->Width(), src->Height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST); 
        glCheckError();
    } else {
        glReadBuffer(GL_COLOR_ATTACHMENT0 + srcPass); 
        glCheckError();
        glBlitFramebuffer(0, 0, src->Width(), src->Height(), 0, 0, src->Width(), src->Height(), GL_COLOR_BUFFER_BIT, GL_NEAREST); 
        glCheckError();
    }
}

bool OpenGLGraphicsDevice::FramebufferCreate(Framebuffer& frambuffer, FrameBufferSpecification specification){
    if(frambuffer.type == FramebufferType::Stand){
        specification.colorAttachments = {
            {FramebufferTextureFormat::RGBA8}
        };
        specification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
        specification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
        specification.sample = 1;
    }
    if(frambuffer.type == FramebufferType::Shadowmap){
        specification.type = FramebufferAttachmentType::TEXTURE_2D_ARRAY;
        specification.depthAttachment = {FramebufferTextureFormat::DEPTH_COMPONENT};
    }
    if(frambuffer.type == FramebufferType::Deffered){
        specification.colorAttachments = {
            {FramebufferTextureFormat::RGB32F}, // Pos
            {FramebufferTextureFormat::RGB32F}, // Normal
            {FramebufferTextureFormat::RGBA16F}, // Albedo
            {FramebufferTextureFormat::RGB16F}, // Emission
            {FramebufferTextureFormat::RGB16F}, // Spec, Metalic, AO
            {FramebufferTextureFormat::RED_INTEGER} // Object ID
        };
        specification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
        specification.type = FramebufferAttachmentType::TEXTURE_2D; //TEXTURE_2D_MULTISAMPLE
        specification.sample = 1;
    }
    frambuffer.specification = specification;

    auto GenColorAttachment = [&](int index){
        Assert(IsDepthTypeFormat(specification.colorAttachments[index].colorFormat) == false);
    
        GLenum internalFormat = InternalFormatLookup[(int)specification.colorAttachments[index].colorFormat];
        GLenum format = FormatLookup[(int)specification.colorAttachments[index].colorFormat];
    
        bool multisample = specification.sample > 1;
    
        unsigned int colorAttachment;
        glGenTextures(1, &colorAttachment);
        glCheckError();
    
        bool hdr = false;
        if(specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGB16F) hdr = true;
        if(specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGB32F) hdr = true;
        if(specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGBA16F) hdr = true;
        if(specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGBA32F) hdr = true;
    
        if(specification.type == FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE){
            #if defined(OpenGLEmscripten)
            Assert(false && "not supported");
            #else
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, specification.sample, internalFormat, specification.width, specification.height, GL_TRUE);
            glCheckError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D_MULTISAMPLE, colorAttachment, 0);
            glCheckError();
            #endif
        } else if(specification.type == FramebufferAttachmentType::TEXTURE_2D){
            glBindTexture(GL_TEXTURE_2D, colorAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, specification.width, specification.height, 0, format, hdr ? GL_FLOAT : GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glCheckError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, colorAttachment, 0);
            glCheckError();
        } else if(specification.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY){
            //Assert(false);
    
            glBindTexture(GL_TEXTURE_2D_ARRAY, colorAttachment);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, specification.width, specification.height, specification.sample, 0, format, hdr ? GL_FLOAT : GL_UNSIGNED_BYTE, NULL);
            glCheckError();
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glCheckError();
            //#if defined(OpenGLEmscripten)
            //Assert(false && "not supported");
            //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, colorAttachment, 0);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, colorAttachment, 0, 0);
            ///*#else
            //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, colorAttachment, 0);
            //#endif*/
            glCheckError();
        }
    
        frambuffer.glData.colorAttachments.push_back(colorAttachment);
    };
    
    auto GenDepthAttachment = [&](){
        if(specification.depthAttachment.colorFormat == FramebufferTextureFormat::None) return;
    
        Assert(IsDepthTypeFormat(specification.depthAttachment.colorFormat) == true);
    
        GLenum internalFormat = InternalFormatLookup[(int)specification.depthAttachment.colorFormat]; //GLenum internalFormat = GL_DEPTH24_STENCIL8;
        GLenum format = FormatLookup[(int)specification.depthAttachment.colorFormat]; //GLenum format = GL_DEPTH_STENCIL;
        GLenum type = GL_UNSIGNED_INT_24_8;
        GLenum attachment = GL_DEPTH_STENCIL_ATTACHMENT;
    
        if(specification.depthAttachment.colorFormat == FramebufferTextureFormat::DEPTH4STENCIL8){
            internalFormat = GL_DEPTH24_STENCIL8;
            format = GL_DEPTH_STENCIL;
            type = GL_UNSIGNED_INT_24_8;
            attachment = GL_DEPTH_STENCIL_ATTACHMENT;
        }
        
        if(specification.depthAttachment.colorFormat == FramebufferTextureFormat::DEPTH_COMPONENT){
            internalFormat = GL_DEPTH_COMPONENT32F;
            format = GL_DEPTH_COMPONENT;
            type = GL_FLOAT;
            attachment = GL_DEPTH_ATTACHMENT;
        }
    
        bool multisample = specification.sample > 1;
    
        glGenTextures(1, &frambuffer.glData.depthAttachment);
    
        if(specification.type == FramebufferAttachmentType::TEXTURE_2D_MULTISAMPLE){
            #if defined(OpenGLEmscripten)
            Assert(false && "not supported");
            #else
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, frambuffer.glData.depthAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, specification.sample, internalFormat, specification.width, specification.height, GL_TRUE);
            glCheckError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE, frambuffer.glData.depthAttachment, 0);
            glCheckError();
            #endif
        } else if(specification.type == FramebufferAttachmentType::TEXTURE_2D){
            glBindTexture(GL_TEXTURE_2D, frambuffer.glData.depthAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, specification.width, specification.height, 0, format, type, NULL);
            glCheckError();
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  
            glCheckError();
    
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, frambuffer.glData.depthAttachment, 0);
            glCheckError();
        } else if(specification.type == FramebufferAttachmentType::TEXTURE_2D_ARRAY){
            //Assert(false);
            glBindTexture(GL_TEXTURE_2D_ARRAY, frambuffer.glData.depthAttachment);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, specification.width, specification.height, specification.sample, 0, format, type, NULL);
            glCheckError();
    
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
            
            //#if defined(OpenGLEmscripten)
            //Assert(false && "not supported");
            //glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, frambuffer.glData.depthAttachment, 0);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, frambuffer.glData.depthAttachment, 0, 0);
            ///*#else
            //glFramebufferTexture(GL_FRAMEBUFFER, attachment, frambuffer.glData.depthAttachment, 0);
            //#endif*/
            glCheckError();
        }
    };

    //Clean Framebuffer
    //if(renderId) 
    FramebufferDestroy(frambuffer);
    
    //Gen Framebuffer
    glGenFramebuffers(1, &frambuffer.glData.renderId);
    glBindFramebuffer(GL_FRAMEBUFFER, frambuffer.glData.renderId);
    glCheckError();

    //Gen Attachments
    for(int i = 0; i < specification.colorAttachments.size(); i++){
        GenColorAttachment(i);
    }
    GenDepthAttachment();

    if(specification.colorAttachments.size() == 0){
        //glDrawBuffer(GL_NONE);
        //glReadBuffer(GL_NONE);
        const GLenum b = GL_NONE;
        glDrawBuffers(1, &b);
        glReadBuffer(GL_NONE);
        glCheckError();
    } else if(specification.colorAttachments.size() > 1){
        //Assert(specification.colorAttachments.size() <= 4);
		
        std::vector<GLenum> buffers;
        for(int i = 0; i < specification.colorAttachments.size(); i++){
            buffers.push_back(GL_COLOR_ATTACHMENT0+i);
        }
		
        glDrawBuffers(specification.colorAttachments.size(), &buffers[0]);
        glCheckError();
    }

    //Check Erros
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        LogError("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
        Assert(false);
    }
    
    //Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
    return true;
}

void OpenGLGraphicsDevice::FramebufferDestroy(Framebuffer& frambuffer){
    if(frambuffer.glData.renderId == 0) return;

    //Assert(renderId > 0);
    //LogInfo("Framebuffer::Destroy %d", _framebuffer);
    
    glDeleteFramebuffers(1, &frambuffer.glData.renderId);
    glCheckError();

    for(int i = 0; i < frambuffer.specification.colorAttachments.size(); i++){
        //if(_specification.colorAttachments[i].writeOnly){
        //    glDeleteRenderbuffers(1, &_colorAttachments[i]);
        //} else {
            glDeleteTextures(1, &frambuffer.glData.colorAttachments[i]);
        //}
    }
    glCheckError();

    
    glDeleteTextures(1, &frambuffer.glData.depthAttachment);
    
    glCheckError();

    frambuffer.glData.renderId = 0;
    frambuffer.glData.colorAttachments.clear();
    frambuffer.glData.depthAttachment = 0;
}

bool OpenGLGraphicsDevice::FramebufferIsValid(Framebuffer& frambuffer){
    return frambuffer.glData.renderId > 0;
}

void* OpenGLGraphicsDevice::FramebufferColorAttachmentId(Framebuffer& framebuffer, int index){
    Assert(index < framebuffer.glData.colorAttachments.size());
    return (void*)(uint64_t)framebuffer.glData.colorAttachments[index]; 
}

void* OpenGLGraphicsDevice::FramebufferDepthAttachmentId(Framebuffer& framebuffer){
    return (void*)(uint64_t)framebuffer.glData.depthAttachment;
}

int OpenGLGraphicsDevice::FramebufferReadPixel(Framebuffer& frambuffer, int attachmentIndex, int x, int y){
    Assert(attachmentIndex < frambuffer.glData.colorAttachments.size());

    glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
    glCheckError();

    int pixelData;
    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
    glCheckError();
    
    return pixelData;
}

const int TextureFilterLookup[] = {
    GL_NEAREST,
    GL_LINEAR
};

const int TextureFilterLookupMipmap[] = {
    GL_NEAREST_MIPMAP_NEAREST,
    GL_LINEAR_MIPMAP_LINEAR
};

const int TextureWrappingLookupMipmap[] = {
    GL_REPEAT,
    GL_MIRRORED_REPEAT,
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_BORDER
};

const int TextureFormatLookupMipmap[] = {
    GL_NONE,
    GL_RGB,
    GL_RGBA,
    //GL_RGB,
    //GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,

    GL_RED,
    GL_RGB,
    GL_RGBA,
};

const int TextureInternalFormatLookupMipmap[] = {
    GL_NONE,
    GL_RGB,
    GL_RGBA,
    //GL_SRGB,
    //GL_SRGB_ALPHA,

    GL_R8,
    GL_RGB8,
    GL_RGBA8,

    GL_R16,
    GL_RGB16,
    GL_RGBA16,

    GL_R16F,
    GL_RGB16F,
    GL_RGBA16F,

    GL_R32F,
    GL_RGB32F,
    GL_RGBA32F,
};

const int TextureDataTypeFormatLookupMipmap[] = {
    GL_UNSIGNED_BYTE,
    GL_UNSIGNED_INT,
    GL_INT,
    GL_FLOAT
};

void LoadSettings(const char* filePath, Texture2DSetting& settings){
    std::ifstream stream(std::string(filePath) + ".meta");
    if(stream.fail()) return;

    cereal::JSONInputArchive archive{stream};
    archive(CEREAL_NVP(settings));
}

bool OpenGLGraphicsDevice::Texture2DCreate(Texture2D& tex, const std::string path, Texture2DSetting settings){
    auto Texture2DGenerate = [&](unsigned int inWidth, unsigned int inHeight, TextureDataType dataType, void* data){
        tex.width = inWidth;
        tex.height = inHeight;
        
        glGenTextures(1, &tex.glData.id);
        glCheckError();
    
        glBindTexture(GL_TEXTURE_2D, tex.glData.id);
        glTexImage2D(GL_TEXTURE_2D, 0, tex.glData.internalFormat, tex.width, tex.height, 0, tex.glData.imageFormat, TextureDataTypeFormatLookupMipmap[(int)dataType], data);
        glCheckError();
        
        if(tex.mipmap == true) glGenerateMipmap(GL_TEXTURE_2D);
        glCheckError();
    
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex.glData.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex.glData.wrapT);
    
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex.glData.filterMin);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex.glData.filterMax);
        glCheckError();
    
        glBindTexture(GL_TEXTURE_2D, 0);
        glCheckError();
    };

    Texture2DDestroy(tex);

    tex.path = path;// std::string(path);
    tex.settings = settings;
    LoadSettings(tex.path.c_str(), tex.settings);
    tex.glData.wrapS = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    tex.glData.wrapT = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    tex.glData.filterMin = TextureFilterLookup[(int)settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(settings.mipmap){
        tex.glData.filterMin = TextureFilterLookupMipmap[(int)settings.filter];
    }
    tex.glData.filterMax = TextureFilterLookup[(int)settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    tex.mipmap = settings.mipmap;

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* data = stbi_load(tex.path.c_str(), &width, &height, &nrChannels, 0);

    //LogInfo("Chennels %d", nrChannels);

    if(!data){
        LogError("Cannot load file image %s\nSTB Reason: %s\n", tex.path.c_str(), stbi_failure_reason());
        return false;
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(settings.textureFormat == TextureFormat::Auto){
        if(alpha){
            tex.glData.internalFormat = GL_RGBA; //GL_SRGB_ALPHA; //GL_RGBA;
            tex.glData.imageFormat = GL_RGBA;
        } else {
            tex.glData.internalFormat = GL_RGB; //GL_SRGB_ALPHA; //GL_RGBA;
            tex.glData.imageFormat = GL_RGB;
        }
    } else {
        tex.glData.internalFormat = TextureInternalFormatLookupMipmap[(int)settings.textureFormat];
        tex.glData.imageFormat = TextureFormatLookupMipmap[(int)settings.textureFormat];
    }
    
    Texture2DGenerate(width, height, TextureDataType::UnsignedByte, data);
    stbi_image_free(data);
    tex.isComplete = true;
    return true;
}

bool OpenGLGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size, Texture2DSetting settings){
    auto Texture2DGenerate = [&](unsigned int inWidth, unsigned int inHeight, TextureDataType dataType, void* data){
        tex.width = inWidth;
        tex.height = inHeight;
        
        glGenTextures(1, &tex.glData.id);
        glCheckError();
    
        glBindTexture(GL_TEXTURE_2D, tex.glData.id);
        glTexImage2D(GL_TEXTURE_2D, 0, tex.glData.internalFormat, tex.width, tex.height, 0, tex.glData.imageFormat, TextureDataTypeFormatLookupMipmap[(int)dataType], data);
        glCheckError();
        
        if(tex.mipmap == true) glGenerateMipmap(GL_TEXTURE_2D);
        glCheckError();
    
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex.glData.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex.glData.wrapT);
    
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex.glData.filterMin);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex.glData.filterMax);
        glCheckError();
    
        glBindTexture(GL_TEXTURE_2D, 0);
        glCheckError();
    };
    Texture2DDestroy(tex);

    tex.path = "Memory";
    tex.settings = settings;
    tex.glData.wrapS = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    tex.glData.wrapT = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    tex.glData.filterMin = TextureFilterLookup[(int)settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(settings.mipmap){
        tex.glData.filterMin = TextureFilterLookupMipmap[(int)settings.filter];
    }
    tex.glData.filterMax = TextureFilterLookup[(int)settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    tex.mipmap = settings.mipmap;

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* _data = stbi_load_from_memory((const stbi_uc*)data, size, &width, &height, &nrChannels, 0);

    //LogInfo("Chennels %d", nrChannels);

    if(!_data){
        LogError("Cannot load file image %s\nSTB Reason: %s\n", tex.path.c_str(), stbi_failure_reason());
        return false;
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(settings.textureFormat == TextureFormat::Auto){
        if(alpha){
            tex.glData.internalFormat = GL_RGBA; //GL_SRGB_ALPHA; //GL_RGBA;
            tex.glData.imageFormat = GL_RGBA;
        } else {
            tex.glData.internalFormat = GL_RGB; //GL_SRGB_ALPHA; //GL_RGBA;
            tex.glData.imageFormat = GL_RGB;
        }
    } else {
        tex.glData.internalFormat = TextureInternalFormatLookupMipmap[(int)settings.textureFormat];
        tex.glData.imageFormat = TextureFormatLookupMipmap[(int)settings.textureFormat];
    }

    Texture2DGenerate(width, height, TextureDataType::UnsignedByte, _data);
    stbi_image_free(_data);
    tex.isComplete = true;
    return true;
}

bool OpenGLGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size, int width, int height, TextureDataType dataType, Texture2DSetting settings){
    auto Texture2DGenerate = [&](unsigned int inWidth, unsigned int inHeight, TextureDataType dataType, void* data){
        tex.width = inWidth;
        tex.height = inHeight;
        
        glGenTextures(1, &tex.glData.id);
        glCheckError();
    
        glBindTexture(GL_TEXTURE_2D, tex.glData.id);
        glTexImage2D(GL_TEXTURE_2D, 0, tex.glData.internalFormat, tex.width, tex.height, 0, tex.glData.imageFormat, TextureDataTypeFormatLookupMipmap[(int)dataType], data);
        glCheckError();
        
        if(tex.mipmap == true) glGenerateMipmap(GL_TEXTURE_2D);
        glCheckError();
    
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex.glData.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex.glData.wrapT);
    
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex.glData.filterMin);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex.glData.filterMax);
        glCheckError();
    
        glBindTexture(GL_TEXTURE_2D, 0);
        glCheckError();
    };

    Texture2DDestroy(tex);

    tex.path = "Memory";
    tex.settings = settings;
    tex.glData.wrapS = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    tex.glData.wrapT = TextureWrappingLookupMipmap[(int)settings.wrap]; //GL_REPEAT;
    tex.glData.filterMin = TextureFilterLookup[(int)settings.filter];// settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    if(settings.mipmap){
        tex.glData.filterMin = TextureFilterLookupMipmap[(int)settings.filter];
    }
    tex.glData.filterMax = TextureFilterLookup[(int)settings.filter]; //settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    tex.mipmap = settings.mipmap;

    Assert(settings.textureFormat != TextureFormat::Auto);

    tex.glData.internalFormat = TextureInternalFormatLookupMipmap[(int)settings.textureFormat];
    tex.glData.imageFormat = TextureFormatLookupMipmap[(int)settings.textureFormat];
    Texture2DGenerate(width, height, dataType, data);
    tex.isComplete = true;
    return true;
}

void OpenGLGraphicsDevice::Texture2DDestroy(Texture2D& tex){
    if(tex.IsValid() == false) return;

    glDeleteTextures(1, &tex.glData.id);
    tex.glData.id = 0;
    glCheckError();

    tex.isComplete = false;
}

bool OpenGLGraphicsDevice::Texture2DIsValid(Texture2D& tex){
    return tex.glData.id != 0;
}

void* OpenGLGraphicsDevice::Texture2DRenderId(Texture2D& tex){
    return (void*)(uint64_t)tex.glData.id;
}

bool OpenGLGraphicsDevice::Texture2DArrayCreate(Texture2DArray& tex, const std::vector<std::string>& filePaths){
    stbi_set_flip_vertically_on_load(1);

    glGenTextures(1, &tex.glData.id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex.glData.id);
    glCheckError();

    std::vector<unsigned char*> datas;
    
    int width, height, nrComponents;
    unsigned int internalFormat = GL_RGB8;
    unsigned int imageFormat = GL_RGB;
    unsigned int mipLevelCount = 8;
    for(auto& i: filePaths){
        unsigned char* data = stbi_load(i.c_str(), &width, &height, &nrComponents, 0);   // Load the first Image of size 512   X   512  (RGB))

        if(!data){
            LogError("Cannot load file image %s\nSTB Reason: %s\n", tex.path.c_str(), stbi_failure_reason());
        }

        if(nrComponents > 3){
            internalFormat = GL_RGBA8; //GL_SRGB_ALPHA; //GL_RGBA;
            imageFormat = GL_RGBA;
            //mipLevelCount = 1;
        }

        datas.push_back(data);
    }

    #if OpenGLVersion == 4
    Assert(false);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, internalFormat, width, height, filePaths.size());
    glCheckError();
    #endif

    #if OpenGLVersion == 3
    //Fixme: make this complatible with opengl 3.3
    Assert(false);
    #endif
    
    int _i = 0;
    for(auto i: datas){
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, _i, width, height, 1, imageFormat, GL_UNSIGNED_BYTE, i);
        glCheckError();
        _i += 1;
    }

    if(mipLevelCount > 1) glGenerateMipmap(GL_TEXTURE_2D_ARRAY); 
    glCheckError();

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glCheckError();

    for(auto i: datas){
        stbi_image_free(i);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glCheckError();
    return true;
}

void OpenGLGraphicsDevice::Texture2DArrayDestroy(Texture2DArray& tex){
    if(Texture2DArrayIsValid(tex) == false) return;

    glDeleteTextures(1, &tex.glData.id);
    tex.glData.id = 0;
    glCheckError();
}

bool OpenGLGraphicsDevice::Texture2DArrayIsValid(Texture2DArray& tex){
    return tex.glData.id != 0;
}

bool OpenGLGraphicsDevice::CubemapCreateFromFile(
    Cubemap& cubemap,
    const char* right, const char* left, const char* top,
    const char* bottom, const char* front, const char* back
){
    CubemapDestroy(cubemap);

    cubemap.mipmap = true;
    std::vector<const char*> faces;
    faces.push_back(right);
    faces.push_back(left);
    faces.push_back(top);
    faces.push_back(bottom);
    faces.push_back(front);
    faces.push_back(back);

    glGenTextures(1, &cubemap.glData.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.glData.id);
    glCheckError();

    stbi_set_flip_vertically_on_load(0);

    int width, height, nrChannels;
    for(unsigned int i = 0; i < faces.size(); i++){
        unsigned char* data = stbi_load(faces[i], &width, &height, &nrChannels, 0);
        if(data){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            glCheckError();
            stbi_image_free(data);
        } else {
            LogError("Cubemap tex failed to load at path: %s", faces[i]);
            //std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }

        LogInfo("Loading Cubemap: %s", faces[i]);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, cubemap.mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if(cubemap.mipmap) glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glCheckError();

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); 
    glCheckError();
    return true;
}

void OpenGLGraphicsDevice::CubemapDestroy(Cubemap& cubemap){
    if(cubemap.glData.id != 0) glDeleteTextures(1, &cubemap.glData.id);
    cubemap.glData.id = 0;
    glCheckError();
}

bool OpenGLGraphicsDevice::CubemapIsValid(Cubemap& tex){
    return tex.glData.id != 0;
}

// Structure to hold uniform details
/*struct UniformInfo {
    std::string name;
    GLint size;      // Number of elements (for arrays)
    GLint offset;    // Offset in the UBO
    GLenum type;     // Data type (e.g., GL_FLOAT_VEC4)
    GLint arrayStride; // Stride for array elements
};*/

// Function to reflect all uniforms in a block
bool getUniformInfo(GLuint program, const char* blockName, UniformBufferDef& out){
    // Find the uniform block index
    GLuint blockIndex = glGetUniformBlockIndex(program, blockName);
    if (blockIndex == GL_INVALID_INDEX) {
        std::cerr << "Uniform block '" << blockName << "' not found.\n";
        return false;
    }

    // Get number of uniforms in the block
    GLint numUniforms;
    glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUniforms);

    if (numUniforms == 0) {
        std::cerr << "Uniform block '" << blockName << "' has no active uniforms.\n";
        return false;
    }

    GLint totalSize;
    glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &totalSize);
    out.size = totalSize;

    // Get the indices of all uniforms in the block
    std::vector<GLuint> uniformIndices(numUniforms);
    glGetActiveUniformBlockiv(program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, reinterpret_cast<GLint*>(uniformIndices.data()));

    // Get uniform names
    for (int i = 0; i < numUniforms; i++) {
        // Get uniform name length
        GLint nameLength;
        glGetActiveUniformsiv(program, 1, &uniformIndices[i], GL_UNIFORM_NAME_LENGTH, &nameLength);

        // Retrieve the name
        std::vector<char> nameBuffer(nameLength);
        GLsizei length;
        glGetActiveUniformName(program, uniformIndices[i], nameLength, &length, nameBuffer.data());

        // Get uniform properties
        //GLint values[4]; // [size, offset, type, arrayStride]
        //GLenum properties[] = {GL_UNIFORM_SIZE, GL_UNIFORM_OFFSET, GL_UNIFORM_TYPE, GL_UNIFORM_ARRAY_STRIDE};
        GLint size, offset, type, arrayStride;
        glGetActiveUniformsiv(program, 1, &uniformIndices[i], GL_UNIFORM_SIZE, &size);
        glGetActiveUniformsiv(program, 1, &uniformIndices[i], GL_UNIFORM_OFFSET, &offset);
        glGetActiveUniformsiv(program, 1, &uniformIndices[i], GL_UNIFORM_TYPE, &type);
        glGetActiveUniformsiv(program, 1, &uniformIndices[i], GL_UNIFORM_ARRAY_STRIDE, &arrayStride);

        auto getUniformByteSize = [](GLint size, GLenum type) -> size_t {
            static const std::unordered_map<GLenum, size_t> typeSizeMap = {
                {GL_FLOAT, 4},
                {GL_FLOAT_VEC2, 8},
                {GL_FLOAT_VEC3, 12},
                {GL_FLOAT_VEC4, 16},
                {GL_INT, 4},
                {GL_INT_VEC2, 8},
                {GL_INT_VEC3, 12},
                {GL_INT_VEC4, 16},
                {GL_BOOL, 4},
                {GL_BOOL_VEC2, 8},
                {GL_BOOL_VEC3, 12},
                {GL_BOOL_VEC4, 16},
                {GL_FLOAT_MAT2, 16},
                {GL_FLOAT_MAT3, 36},
                {GL_FLOAT_MAT4, 64},
                // Add other types as needed
            };
        
            auto it = typeSizeMap.find(type);
            if (it != typeSizeMap.end()) {
                return size * it->second;
            } else {
                // Handle unknown type
                return 0;
            }
        };

        // Store the uniform info
        /*UniformInfo info;
        info.name = std::string(nameBuffer.data(), length);
        info.size = getUniformByteSize(size, type);// size; //values[0];
        info.offset = offset;// values[1];
        info.type = type;// values[2];
        info.arrayStride = arrayStride;// values[3];
        out.push_back(info);*/

        auto removeZeroSubscript = [](std::string& str) {
            const std::string target = "[0]";
            size_t pos = std::string::npos;
            while ((pos = str.find(target)) != std::string::npos) {
                str.erase(pos, target.length());
            }
        };

        std::string label = std::string(nameBuffer.data(), length);
        removeZeroSubscript(label);

        UniformBufferDef::Member m = {};
        m.pos = offset;
        m.size = getUniformByteSize(size, type);
        out.members[label] = m;
    }

    return true;
}

bool OpenGLGraphicsDevice::SubShaderCreateFromBaseSource(
    SubShader& shader,
    std::string& source, 
    std::vector<std::string>& keyworlds,
    ShaderPipeline pipeline, 
    std::vector<std::string>& errors
){
    SubShaderDestroy(shader);
    shader.enabledKeyworlds = keyworlds;
    shader.pipeline = pipeline;

    //std::string vertexToInsert = "#version 330 core\n#define VERTEX\n";
    //std::string fragToInsert = "#version 330 core\n#define FRAGMENT\n";
    
    #if UseUniformBuffer
    std::string vertexToInsert = OpenglHeader "\n#define VERTEX\n#define UseUniformBuffer\n#define OpenGL_API\n";
    std::string fragToInsert = OpenglHeader "\n#define FRAGMENT\n#define UseUniformBuffer\n#define OpenGL_API\n";
    #else
    std::string vertexToInsert = OpenglHeader "\n#define VERTEX\n#define OpenGL_API\n";
    std::string fragToInsert = OpenglHeader "\n#define FRAGMENT\n#define OpenGL_API\n";
    #endif

    auto CompileShader = [&](std::string& baseSource, std::string& toInsert, GLenum type, GLuint program, GLenum& shader) -> bool{
        baseSource.insert(0, toInsert);
        //LogWarning("%s", baseSource.c_str());

        shader = glCreateShader(type);

        const GLchar* sourceCStr = baseSource.c_str();
        glShaderSource(shader, 1, &sourceCStr, 0);
        glCompileShader(shader);
        glCheckError();

        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if(isCompiled == GL_FALSE){
            GLint maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
            glCheckError();

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
            glCheckError();

            glDeleteShader(shader);
            glCheckError();

            errors.push_back(std::string(&infoLog[0]));

            //printf("%s", infoLog.data());
            //Assert(false && "Shader compilation failure!");
            LogError("Shader compilation failure!");
            LogError("%s", infoLog.data());
            baseSource.erase(0, toInsert.size());
            return false;
        }

        glAttachShader(program, shader);
        glCheckError();
        baseSource.erase(0, toInsert.size());
        return true;
    };

    GLuint program = glCreateProgram();
    glCheckError();
    
    GLenum glShaderIDs[2] = {0, 0};
    //int glShaderIDIndex = 0;

    if(CompileShader(source, vertexToInsert, GL_VERTEX_SHADER, program, glShaderIDs[0]) == false) return false;
    if(CompileShader(source, fragToInsert, GL_FRAGMENT_SHADER, program, glShaderIDs[1]) == false) return false;
    
    shader.glData.id = program;

    // Link our program
    glLinkProgram(program);
    glCheckError();

    // Note the different functions here: glGetProgram* instead of glGetShader*.
    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
    glCheckError();

    if (isLinked == GL_FALSE){
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        glCheckError();

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
        glCheckError();

        // We don't need the program anymore.
        glDeleteProgram(program);
        glCheckError();
        
        for(auto id : glShaderIDs){
            glDeleteShader(id);
            glCheckError();
        }

        printf("%s", infoLog.data());
        LogError("Shader link failure!");// LogError("Shader link failure! %s", shader.path.c_str());
        Assert(false && "Shader link failure!");
        return false;
    }

    for(auto id: glShaderIDs){
        if(id == 0) continue;
        glDetachShader(program, id);
        glCheckError();
    }

    glCheckError();

    GLint count;
    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)
    const GLsizei bufSize = 64; // maximum name length
    GLchar name[bufSize]; // variable name in GLSL
    GLsizei length; // name length
    glGetProgramiv(shader.glData.id, GL_ACTIVE_UNIFORMS, &count);
    //printf("Active Uniforms: %d\n", count);
    for(int i = 0; i < count; i++){
        glGetActiveUniform(shader.glData.id, (GLuint)i, bufSize, &length, &size, &type, name);
        
        std::string t = std::string(name);
        std::string s = "[0]";
        std::string::size_type _i = t.find(s);
        if (_i != std::string::npos)
        t.erase(_i, s.length());

        /*GLint location = glGetUniformLocation(shader.glData.id, t.c_str());
        glCheckError();
        shader.glData.uniforms[name] = location;*/

        shader.glData._uniforms.push_back(std::string(t));
        //printf("Uniform #%d Type: %u Name: %s\n", i, type, name);
    }

    /*UniformBufferDef mainBufferDef;
    if(getUniformInfo(program, "MaterialData", mainBufferDef)){
        LogInfo("--------TotalSize %zd ---------------", mainBufferDef.size);
        for(auto& i: mainBufferDef.members){
           LogInfo("Name: %s", i.first.c_str());
           LogInfo("Pos: %zd", i.second.pos);
           LogInfo("Offset: %zd", i.second.size);
        }
    }*/

    return true;
}

void OpenGLGraphicsDevice::SubShaderDestroy(SubShader& shader){
    //if(shader.IsValid() == false) return;

    glDeleteProgram(shader.glData.id);
    glCheckError();
    shader.glData.id = 0;
}

bool OpenGLGraphicsDevice::SubShaderIsValid(SubShader& shader){
    return shader.glData.id != 0;
}

void OpenGLGraphicsDevice::SubShaderBind(SubShader& shader){
    SetColorMask(shader.pipeline.colorMask);
    SetCullFace(shader.GetCullFace());
    SetDepthTest(shader.GetDepthTest());
    SetDepthMask(shader.IsDepthMask());
    if(shader.IsBlend()){
        SetBlend(true);
        SetBlendFunc(shader.GetSrcBlend(), shader.GetDstBlend());
    } else {
        SetBlend(false);
    }

    glUseProgram(shader.glData.id);
    glCheckError();
    
    stats.shaderBinds += 1;
}

bool OpenGLGraphicsDevice::ShaderCreate(Shader& shader, std::string inPath){
    //LogInfo("Create Shader: %s", inPath.c_str());
    return shader.Create(inPath);
}

void OpenGLGraphicsDevice::ShaderDestroy(Shader& shader){
    shader.Destroy();
}

bool OpenGLGraphicsDevice::MaterialCreate(Material& shader){
    return false;
}

void OpenGLGraphicsDevice::MaterialDestroy(Material& shader){

}

void OpenGLGraphicsDevice::MaterialOnSetShader(Material& mat){
    #if UseUniformBuffer
    Assert(mat.currentShader != nullptr);

    if(getUniformInfo(mat.currentShader->glData.id, "Main", mat.glData.mainBufferDef)){
        mat.glData.mainUniformData = malloc(mat.glData.mainBufferDef.size);
        memset(mat.glData.mainUniformData, 0, mat.glData.mainBufferDef.size);
        glGenBuffers(1, &mat.glData.mainBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, mat.glData.mainBuffer);
        glCheckError();

        /*LogInfo("--------TotalSize %zd ---------------", mat.glData.mainBufferDef.size);
        for(auto& i: mat.glData.mainBufferDef.members){
           LogInfo("Name: %s", i.first.c_str());
           LogInfo("Pos: %zd", i.second.pos);
           LogInfo("Size: %zd", i.second.size);
        }*/
    }
    #endif
}

void OpenGLGraphicsDevice::MaterialOnUnsetShader(Material& shader){}

bool OpenGLGraphicsDevice::ImGuiSupport(){
    return true;
}

void OpenGLGraphicsDevice::ImGuiNewFrame(){
    ImGui_ImplOpenGL3_NewFrame();
}

void OpenGLGraphicsDevice::ImGuiRenderDrawData(unsigned int x, unsigned int y, unsigned int w, unsigned int h){
    ImVec4 _clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    glViewport(0, 0, w, h);
    if(ImGuiLayer::GetCleanAll() == true){
        glClearColor(_clear_color.x * _clear_color.w, _clear_color.y * _clear_color.w, _clear_color.z * _clear_color.w, _clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}
#endif