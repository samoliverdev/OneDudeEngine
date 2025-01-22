#include "Graphics.h"
#include "OD/Defines.h"
#include "OD/Platform/GL.h"
#include "OD/Core/Lua.h"
#include "OD/Graphics/SubShader.h"
#include "OD/Graphics/Camera.h"

//#define ENGINE_RESOURCE_PATH "res/Engine/"

namespace OD{

void GraphicsModuleInit(){
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".png", [](const std::string& path){ return AssetManager::Get().LoadAsset<Texture2D>(path); });
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".jpg", [](const std::string& path){ return AssetManager::Get().LoadAsset<Texture2D>(path); });
    AssetTypesDB::Get().RegisterAssetType<Material>(".material", [](const std::string& path){ return AssetManager::Get().LoadAsset<Material>(path); });

    LuaBindsDB::Get().RegisterLuaBind<Camera>();
    LuaBindsDB::Get().RegisterLuaBind<Cubemap>();
    LuaBindsDB::Get().RegisterLuaBind<Font>();
    LuaBindsDB::Get().RegisterLuaBind<Framebuffer>();
    LuaBindsDB::Get().RegisterLuaBind<Graphics>();
    LuaBindsDB::Get().RegisterLuaBind<Material>();
    LuaBindsDB::Get().RegisterLuaBind<Texture2D>();
}

unsigned int globalVAO;

unsigned int lineVAO;
unsigned int lineVBO;
unsigned int lineCommandsVAO;
unsigned int lineCommandsVBO;
std::vector<float> lineCommandsData;
#define MAX_LINES_VERTEX_DRAWCALL 1000000

unsigned int textQuadVAO;
unsigned int textQuadVBO;

unsigned int wiredCubeVAO;
unsigned int wiredCubeVBO;
unsigned int wiredCubeEBO;

Ref<SubShader> gismoShader;
Ref<Mesh> fullScreenQuad;
Camera camera;

int drawCalls;
int vertices;
int tris;
int shaderBinds;
int uniformSet;

bool begin = false;

GLenum meshDrawModeLookup[] = {
    GL_TRIANGLES,
    GL_LINES,
    GL_POINTS,
    //GL_QUADS
};  

int Graphics::GetDrawCallsCount(){ return drawCalls; }
int Graphics::GetVerticesCount(){ return vertices; }
int Graphics::GetTrisCount(){ return tris; }
int Graphics::GetShaderBinds(){ return shaderBinds; }
int Graphics::GetUniformSet(){ return uniformSet; }

void CreateLineVAO(unsigned int* vao, unsigned int* vbo, int vertexCount){
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
}

void CreateWiredCubeVAO(){
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
}

void CreateTextQuadVAO(){
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
}

void Graphics::Initialize(){
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
    gismoShader = SubShader::CreateFromFile("Engine/Shaders/Gizmos.glsl");
    Assert(gismoShader != nullptr);

    CreateLineVAO(&lineVAO, &lineVBO, 2);
    CreateLineVAO(&lineCommandsVAO, &lineCommandsVBO, MAX_LINES_VERTEX_DRAWCALL*2);
    CreateWiredCubeVAO();
    CreateTextQuadVAO();

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
}

void Graphics::Shutdown(){
    fullScreenQuad = nullptr;
    gismoShader = nullptr;
}

GLsync sync = nullptr;

void Graphics::Begin(){
    drawCalls = 0;
    vertices = 0;
    tris = 0;
    shaderBinds = 0;
    uniformSet = 0;
    begin = true;

    //if(sync != nullptr) glClientWaitSync(sync, 0, 0);
    //sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void Graphics::End(){
    begin = false;
}

void Graphics::_Begin(){
    #ifndef USE_VAO
    glBindVertexArray(globalVAO);
    glCheckError();
    #endif
}

void Graphics::_End(){
    #ifndef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif
}

bool Graphics::HasBegin(){
    return begin;
}

void Graphics::Clean(float r, float g, float b, float a){
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
}

void Graphics::SetCamera(Camera& inCamera){
    camera = inCamera;
}

Camera Graphics::GetCamera(){
    return camera;
}

void Graphics::SetProjectionViewMatrix(SubShader& shader){
    shader.SetMatrix4("view", camera.view);
    shader.SetMatrix4("projection", camera.projection);
}

void Graphics::SetModelMatrix(SubShader& shader, Matrix4 modelMatrix){
    shader.SetMatrix4("model", modelMatrix);
}

/*void Renderer::DrawMeshRaw(Mesh& mesh){
    Assert(mesh.IsValid() && "Mesh is not vali!");

    drawCalls += 1;
    vertices += mesh._vertexCount;
    tris += mesh._indiceCount;

    glBindVertexArray(mesh._vao);
    glCheckError();

    if(mesh._ebo != 0){
        glDrawElements(GL_TRIANGLES, mesh._indiceCount, GL_UNSIGNED_INT, 0);
        glCheckError();
    } else {
        glDrawArrays(GL_TRIANGLES, 0, mesh._vertexCount);
        glCheckError();
    }

    glBindVertexArray(0);
    glCheckError();
}*/

void Graphics::DrawMeshRaw(Mesh& mesh){
    if(mesh.IsValid() == false){
        #ifdef GRAPHIC_LOG_ERROR
        LogError("DrawMesh::InvalidMesh");
        #endif
        return;
    }

    Assert(mesh.IsValid() && "Mesh is not vali!");
    //Assert(shader.IsValid()  && "Shader is not vali!");

    drawCalls += 1;
    vertices += mesh.vertexCount;
    tris += mesh.indiceCount;
    
    #ifdef USE_VAO
    glBindVertexArray(mesh.vao);
    glCheckError();
    #else
    mesh.Bind();
    #endif

    if(mesh.ebo != 0){
        glDrawElements(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0);
        glCheckError();
    } else {
        glDrawArrays(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount);
        glCheckError();
    }

    //glBindVertexArray(0);
    //glCheckError();
}

void Graphics::DrawMeshInstancingRaw(Mesh& mesh, int count){
    Assert(mesh.IsValid() && "Mesh is not vali!");

    drawCalls += 1;
    vertices += mesh.vertexCount * count;
    tris += mesh.indiceCount * count;

    #ifdef USE_VAO
    glBindVertexArray(mesh.vao);
    #else
    mesh.Bind();
    #endif

    if(mesh.ebo != 0){
        glDrawElementsInstanced(meshDrawModeLookup[(int)mesh.drawMode], mesh.indiceCount, GL_UNSIGNED_INT, 0, count);
        glCheckError();
    } else {
        glDrawArraysInstanced(meshDrawModeLookup[(int)mesh.drawMode], 0, mesh.vertexCount, count);
        glCheckError();
    }

    //glBindVertexArray(0);
    //glCheckError();
}

void Graphics::DrawMesh(Mesh& mesh, SubShader& shader, Matrix4 modelMatrix){
    SubShader::Bind(shader);
    shader.SetMatrix4("projection", camera.projection);
    shader.SetMatrix4("view", camera.view);
    shader.SetMatrix4("model", modelMatrix);
    Graphics::DrawMeshRaw(mesh);
}

void Graphics::DrawMeshInstancing(Mesh& mesh, SubShader& shader, Matrix4* modelMatrixs, int count){
    mesh.SubmitInstancingCustomModelMatrixs(modelMatrixs, count);
    SubShader::Bind(shader);
    shader.SetMatrix4("projection", camera.projection);
    shader.SetMatrix4("view", camera.view);
    Graphics::DrawMeshInstancingRaw(mesh, count);
}

void Graphics::DrawModel(Model& model, Matrix4 modelMatrix){
    int index = 0;
    for(auto i: model.renderTargets){
        Ref<Material> targetMaterial = model.materials[i.materialIndex];
        Ref<Mesh> targetMesh = model.meshs[i.meshIndex];
        Matrix4 targetMatrix =  modelMatrix * model.skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
        Material::SubmitGraphicDatas(*targetMaterial);
        DrawMesh(*targetMesh, *targetMaterial->GetShader(), targetMatrix);
    }
}

void Graphics::AddDrawLineCommand(Vector3 start, Vector3 end){
    lineCommandsData.push_back(start.x);
    lineCommandsData.push_back(start.y);
    lineCommandsData.push_back(start.z);

    lineCommandsData.push_back(end.x);
    lineCommandsData.push_back(end.y);
    lineCommandsData.push_back(end.z);
}

void Graphics::DrawLinesComamnd(Vector3 color, int lineWidth){
    //drawCalls += 1;
    vertices += lineCommandsData.size()/3;
    tris += 0;

    SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);

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

    for (int i = 0; i < lineCommandsData.size(); i += 2 * MAX_LINES_VERTEX_DRAWCALL * 3){
        drawCalls += 1;
        int batchVertexCount = std::min<int>(lineCommandsData.size() - i, 2 * MAX_LINES_VERTEX_DRAWCALL * 3);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * batchVertexCount, &lineCommandsData[i]);
	    glCheckError();
        glDrawArrays(GL_LINES, 0, batchVertexCount/3);
        glCheckError();
    }

    lineCommandsData.clear();
}

void Graphics::DrawLine(Vector3 start, Vector3 end, Vector3 color, int width){
    drawCalls += 1;
    vertices += 2;
    tris += 0;

    SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);

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

void Graphics::DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int width){
    drawCalls += 1;
    vertices += 2;
    tris += 0;

    start = Vector3(model * Vector4(start.x, start.y, start.z, 1));
    end = Vector3(model * Vector4(end.x, end.y, end.z, 1));

    SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity); //gismoShader->SetMatrix4("model", model);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);

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

void Graphics::DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth){
    drawCalls += 1;
    vertices += 8;
    tris += 24;

    SubShader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", modelMatrix);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);

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

void Graphics::DrawText(Font& f, SubShader& s, std::string text, Vector3 pos, float scale){
    SubShader::Bind(s);
    //s.SetVector4("color", color);
    s.SetMatrix4("projection", camera.projection);
    s.SetMatrix4("view", camera.view);
    //s.SetMatrix4("view", Matrix4Identity);

    Transform t(pos);
    s.SetMatrix4("model", t.GetLocalModelMatrix());

    glActiveTexture(GL_TEXTURE0);

    #ifdef USE_VAO
    glBindVertexArray(textQuadVAO);
    glCheckError();
    #else
    glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glCheckError();
    #endif

    int x = 0; //pos.x;
    int y = 0; //pos.y;

    // iterate through all characters
    std::string::const_iterator c;
    for(c = text.begin(); c != text.end(); c++){
        Character ch = f.characters[*c];

        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;
        float zpos = 0;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        // update VBO for each character
        float vertices[6][5] = {
            { xpos,     ypos + h, zpos,   0.0f, 0.0f },            
            { xpos,     ypos,     zpos,   0.0f, 1.0f },
            { xpos + w, ypos,     zpos,   1.0f, 1.0f },

            { xpos,     ypos + h, zpos,   0.0f, 0.0f },
            { xpos + w, ypos,     zpos,   1.0f, 1.0f },
            { xpos + w, ypos + h, zpos,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glCheckError();

        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glCheckError();

        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glCheckError();
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }

    #ifdef USE_VAO
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
    #endif
}

void Graphics::DrawText(Font& f, SubShader& s, std::string text, Matrix4 model){
    SubShader::Bind(s);
    //s.SetVector4("color", color);
    s.SetMatrix4("projection", camera.projection);
    s.SetMatrix4("view", camera.view);
    s.SetMatrix4("model", model);

    glActiveTexture(GL_TEXTURE0);

    #ifdef USE_VAO
    glBindVertexArray(textQuadVAO);
    glCheckError();
    #else
    glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glCheckError();
    #endif

    int x = 0; //pos.x;
    int y = 0; //pos.y;
    float scale = 1;

    // iterate through all characters
    std::string::const_iterator c;
    for(c = text.begin(); c != text.end(); c++){
        Character ch = f.characters[*c];

        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;
        float zpos = 0;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        // update VBO for each character
        float vertices[6][5] = {
            { xpos,     ypos + h, zpos,   0.0f, 0.0f },            
            { xpos,     ypos,     zpos,   0.0f, 1.0f },
            { xpos + w, ypos,     zpos,   1.0f, 1.0f },

            { xpos,     ypos + h, zpos,   0.0f, 0.0f },
            { xpos + w, ypos,     zpos,   1.0f, 1.0f },
            { xpos + w, ypos + h, zpos,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glCheckError();

        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glCheckError();

        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glCheckError();
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }

    #ifdef USE_VAO
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
    #endif
}

void Graphics::SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){
    glViewport(x, y, w, h);
}

void Graphics::GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h){
    GLint value[4];
    glGetIntegerv(GL_VIEWPORT, value);
    *x = value[0]; 
    *y = value[1];
    *w = value[2]; 
    *h = value[3];
}

void Graphics::SetColorMask(float r, float g, float b, float a){
    glColorMask(r, g, b, a);
}   

void Graphics::SetRenderMode(RenderMode mode){
    if(mode == RenderMode::SHADED) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if(mode == RenderMode::WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glCheckError();
}

void Graphics::SetDepthMask(bool value){
    if(value){
        glDepthMask(GL_TRUE);
    } else {
        glDepthMask(GL_FALSE);
    }
    glCheckError();
}

void Graphics::SetDepthTest(DepthTest depthTest){
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

void Graphics::SetCullFace(CullFace cullFace){
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

void Graphics::SetBlend(bool b){
    if(b){
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    glCheckError();
}

int BlendModeToGL(BlendMode blendMode){
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

void Graphics::SetBlendFunc(BlendMode sfactor, BlendMode dfactor){
    glBlendFunc(BlendModeToGL(sfactor), BlendModeToGL(dfactor));
    glCheckError();
}

void Graphics::BeginFramebuffer(Framebuffer* framebuffer){
    Framebuffer::Bind(*framebuffer);
}

void Graphics::BlitQuadPostProcessing(Framebuffer* src, Framebuffer* dst, SubShader& shader, int pass){
    Assert(src != nullptr);

    if(dst == nullptr){
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        Framebuffer::Bind(*dst);
        Graphics::SetViewport(0, 0, dst->Width(), dst->Height());
    }
    glCheckError();
    
    Graphics::Clean(1,1,1,1);
    Graphics::SetDepthTest(DepthTest::DISABLE); 
    glCheckError();

    SubShader::Bind(shader);
    shader.SetFramebuffer("mainTex", *src, 0, pass);
    //src->BindColorAttachmentTexture(shader, 0);

    Graphics::DrawMeshRaw(*fullScreenQuad);
    glCheckError();
}

void Graphics::BlitQuadPostProcessingRaw(Framebuffer* dst){
    if(dst == nullptr){
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        Framebuffer::Bind(*dst);
        Graphics::SetViewport(0, 0, dst->Width(), dst->Height());
    }
    glCheckError();
    
    Graphics::Clean(1,1,1,1);
    Graphics::SetDepthTest(DepthTest::DISABLE); 
    glCheckError();

    //src->BindColorAttachmentTexture(shader, 0);
    Graphics::DrawMeshRaw(*fullScreenQuad);
    glCheckError();
}

void Graphics::BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass){
    Assert(src != nullptr);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, src->RenderId());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst == nullptr ? 0 : dst->RenderId());
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

void Graphics::CreateLuaBind(sol::state& lua){
    lua.new_enum(
        "DepthTest",
        "DISABLE", DepthTest::DISABLE,
        "LESS", DepthTest::LESS,
        "LESS_EQUAL", DepthTest::LESS_EQUAL,
        "EQUAL", DepthTest::EQUAL,
        "GREATER", DepthTest::GREATER,
        "GREATER_EQUAL", DepthTest::GREATER_EQUAL,
        "DIFFERENT", DepthTest::DIFFERENT,
        "NEVER", DepthTest::NEVER,
        "ALWAYS", DepthTest::ALWAYS
    );
    lua.new_enum(
        "CullFace",
        "NONE", CullFace::NONE,
        "BACK", CullFace::BACK,
        "FRONT", CullFace::FRONT,
        "FRONT_AND_BACK", CullFace::FRONT_AND_BACK
    );
    lua.new_enum(
        "BlendMode",
        "ZERO", BlendMode::ZERO,
        "ONE", BlendMode::ONE,
        "SRC_COLOR", BlendMode::SRC_COLOR,
        "ONE_MINUS_SRC_COLOR", BlendMode::ONE_MINUS_SRC_COLOR,
        "DST_COLOR", BlendMode::DST_COLOR,
        "ONE_MINUS_DST_COLOR", BlendMode::ONE_MINUS_DST_COLOR,
        "SRC_ALPHA", BlendMode::SRC_ALPHA,
        "ONE_MINUS_SRC_ALPHA", BlendMode::ONE_MINUS_SRC_ALPHA,
        "DST_ALPHA", BlendMode::DST_ALPHA,
        "ONE_MINUS_DST_ALPHA", BlendMode::ONE_MINUS_DST_ALPHA,
        "CONSTANT_COLOR", BlendMode::CONSTANT_COLOR,
        "ONE_MINUS_CONSTANT_COLOR", BlendMode::ONE_MINUS_CONSTANT_COLOR,
        "CONSTANT_ALPHA", BlendMode::CONSTANT_ALPHA,
        "ONE_MINUS_CONSTANT_ALPHA", BlendMode::ONE_MINUS_CONSTANT_ALPHA
    );
    lua.new_enum(
        "GraphicsRenderMode",
        "SHADED", Graphics::RenderMode::SHADED,
        "WIREFRAME", Graphics::RenderMode::WIREFRAME
    );
    lua.new_usertype<Graphics>(
        "Graphics",
        "GetDrawCallsCount", Graphics::GetDrawCallsCount,
        "GetVerticesCount", Graphics::GetVerticesCount,
        "GetTrisCount", Graphics::GetTrisCount,
        "Begin", Graphics::Begin,
        "End", Graphics::End,
        "HasBegin", Graphics::HasBegin,
        "Clean", Graphics::Clean,
        "SetCamera", Graphics::SetCamera,
        "GetCamera", Graphics::GetCamera,
        "SetProjectionViewMatrix", Graphics::SetProjectionViewMatrix,
        "SetModelMatrix", Graphics::SetModelMatrix,
        "DrawMeshRaw", Graphics::DrawMeshRaw,
        "DrawMeshInstancingRaw", Graphics::DrawMeshInstancingRaw,
        "DrawMesh", Graphics::DrawMesh,
        "DrawMeshInstancing", Graphics::DrawMeshInstancing,
        "DrawModel", Graphics::DrawModel,
        "AddDrawLineCommand", Graphics::AddDrawLineCommand,
        "DrawLinesComamnd", Graphics::DrawLinesComamnd,
        "DrawLine", sol::overload(
            [](Vector3 start, Vector3 end, Vector3 color, int lineWidth){ Graphics::DrawLine(start, end, color, lineWidth);},
            [](Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth){ Graphics::DrawLine(model, start, end, color, lineWidth);}
        ),
        "DrawText", sol::overload(
            [](Font& f, SubShader& s, std::string text, Vector3 pos, float scale){ Graphics::DrawText(f, s, text, pos, scale); },
            [](Font& f, SubShader& s, std::string text, Matrix4 model){ Graphics::DrawText(f, s, text, model); }
        ),
        "SetViewport", Graphics::SetViewport,
        "GetViewport", Graphics::GetViewport,
        "SetRenderMode", Graphics::SetRenderMode,
        "SetDepthMask", Graphics::SetDepthMask,
        "SetDepthTest", Graphics::SetDepthTest,
        "SetCullFace", Graphics::SetCullFace,
        "SetBlend", Graphics::SetBlend,
        "SetBlendFunc", Graphics::SetBlendFunc,
        "BeginFramebuffer", Graphics::BeginFramebuffer,
        "BlitQuadPostProcessing", Graphics::BlitQuadPostProcessing,
        "BlitFramebuffer", Graphics::BlitFramebuffer
    );
}

}