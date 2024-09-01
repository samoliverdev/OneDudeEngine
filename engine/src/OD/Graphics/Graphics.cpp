#include "Graphics.h"
#include "OD/Defines.h"
#include "OD/Platform/GL.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/Camera.h"

//#define ENGINE_RESOURCE_PATH "res/Engine/"

namespace OD{

void GraphicsModuleInit(){
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".png", [](const std::string& path){ return AssetManager::Get().LoadAsset<Texture2D>(path); });
    AssetTypesDB::Get().RegisterAssetType<Texture2D>(".jpg", [](const std::string& path){ return AssetManager::Get().LoadAsset<Texture2D>(path); });
    AssetTypesDB::Get().RegisterAssetType<Material>(".material", [](const std::string& path){ return AssetManager::Get().LoadAsset<Material>(path); });
}

unsigned int lineVAO;
unsigned int lineVBO;
unsigned int lineCommandsVAO;
unsigned int lineCommandsVBO;
std::vector<float> lineCommandsData;
#define MAX_LINES_VERTEX_DRAWCALL 1000000

unsigned int textQuadVAO;
unsigned int textQuadVBO;

unsigned int wiredCubeVAO;

Ref<Shader> gismoShader;
Ref<Mesh> fullScreenQuad;
Camera camera;

int drawCalls;
int vertices;
int tris;

bool begin = false;

GLenum meshDrawModeLookup[] = {
    GL_TRIANGLES,
    GL_LINES,
    GL_POINTS,
    GL_QUADS
};  

int Graphics::GetDrawCallsCount(){ return drawCalls; }
int Graphics::GetVerticesCount(){ return vertices; }
int Graphics::GetTrisCount(){ return tris; }

void CreateLineVAO(unsigned int* vao, unsigned int* vbo, int vertexCount){
    glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);
    glCheckError();
    
	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	//glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexCount * 3, NULL, GL_DYNAMIC_DRAW);
    glCheckError();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
    glCheckError();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
    glCheckError();
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

    glGenVertexArrays(1, &wiredCubeVAO);
	glBindVertexArray(wiredCubeVAO);
    glCheckError();
    
    unsigned int vbo;
    unsigned int ebo;

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glCheckError();

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);
    glCheckError();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
    glCheckError();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
    glCheckError();
}

void CreateTextQuadVAO(){
    glGenVertexArrays(1, &textQuadVAO);
    glGenBuffers(1, &textQuadVBO);
    glCheckError();

    glBindVertexArray(textQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 5, NULL, GL_DYNAMIC_DRAW);
    glCheckError();
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glCheckError();

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glCheckError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);   
    glCheckError();
}

void Graphics::Initialize(){
    glEnable(GL_DEPTH_TEST); 

    fullScreenQuad = Mesh::FullScreenQuad();
    //gismoShader = Shader::CreateFromFiles("res/shaders/gizmos.vert", "res/shaders/gizmos.frag");
    gismoShader = Shader::CreateFromFile("Engine/Shaders/Gizmos.glsl");
    Assert(gismoShader != nullptr);

    CreateLineVAO(&lineVAO, &lineVBO, 2);
    CreateLineVAO(&lineCommandsVAO, &lineCommandsVBO, MAX_LINES_VERTEX_DRAWCALL*2);
    CreateWiredCubeVAO();
    CreateTextQuadVAO();

    GLint maxLayers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);
    LogInfo("MaxArrayTextureLayers: %d", maxLayers);

    GLint maxVertexUniformComponents;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniformComponents);
    LogInfo("MaxVertexUniformComponents: %d", maxVertexUniformComponents);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniformComponents);
    LogInfo("MaxVertexUniformComponentVectors: %d", maxVertexUniformComponents);

    GLint maxFragmentUniformComponents;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragmentUniformComponents);
    LogInfo("MaxFragmentUniformComponents: %d", maxFragmentUniformComponents);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxFragmentUniformComponents);
    LogInfo("MaxFragmentUniformComponentVectors: %d", maxFragmentUniformComponents);

    GLint maxTextureUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    LogInfo("MaxTextureUnits: %d", maxTextureUnits);
}

void Graphics::Shutdown(){

}

void Graphics::Begin(){
    drawCalls = 0;
    vertices = 0;
    tris = 0;
    begin = true;
}

void Graphics::End(){
    begin = false;
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

void Graphics::SetProjectionViewMatrix(Shader& shader){
    shader.SetMatrix4("view", camera.view);
    shader.SetMatrix4("projection", camera.projection);
}

void Graphics::SetModelMatrix(Shader& shader, Matrix4 modelMatrix){
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
    
    glBindVertexArray(mesh.vao);
    glCheckError();

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
    
    /*
    shader.Bind();
    shader.SetFloat("useInstancing", 1.0f); 
    //if(shader._uniforms.count("useInstancing") > 0) shader.SetFloat("useInstancing", 1.0f);
    shader.SetMatrix4("view", camera.view);
    shader.SetMatrix4("projection", camera.projection);
    */

    glBindVertexArray(mesh.vao);

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

void Graphics::DrawMesh(Mesh& mesh, Shader& shader, Matrix4 modelMatrix){
    Shader::Bind(shader);
    shader.SetMatrix4("projection", camera.projection);
    shader.SetMatrix4("view", camera.view);
    shader.SetMatrix4("model", modelMatrix);
    Graphics::DrawMeshRaw(mesh);
}

void Graphics::DrawMeshInstancing(Mesh& mesh, Shader& shader, Matrix4* modelMatrixs, int count){
    mesh.SubmitInstancingCustomModelMatrixs(modelMatrixs, count);
    Shader::Bind(shader);
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

    Shader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);

	glLineWidth(lineWidth);
    glCheckError();

	glBindVertexArray(lineCommandsVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineCommandsVBO);
    glCheckError();
    
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

    Shader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);

	glLineWidth(width);

    float line[6] = {start.x, start.y, start.z, end.x, end.y, end.z};

	glBindVertexArray(lineVAO);
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

    Shader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4Identity); //gismoShader->SetMatrix4("model", model);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);

	glLineWidth(width);

    float line[6] = {start.x, start.y, start.z, end.x, end.y, end.z};

	glBindVertexArray(lineVAO);
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

    Shader::Bind(*gismoShader);
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", modelMatrix);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);
    
    glLineWidth(lineWidth);
    glBindVertexArray(wiredCubeVAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    glCheckError();

    //glBindVertexArray(0);
    //glCheckError();
}

void Graphics::DrawText(Font& f, Shader& s, std::string text, Vector3 pos, float scale){
    Shader::Bind(s);
    //s.SetVector4("color", color);
    s.SetMatrix4("projection", camera.projection);
    s.SetMatrix4("view", camera.view);
    //s.SetMatrix4("view", Matrix4Identity);

    Transform t(pos);
    s.SetMatrix4("model", t.GetLocalModelMatrix());

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textQuadVAO);
    glCheckError();

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

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
}

void Graphics::DrawText(Font& f, Shader& s, std::string text, Matrix4 model){
    Shader::Bind(s);
    //s.SetVector4("color", color);
    s.SetMatrix4("projection", camera.projection);
    s.SetMatrix4("view", camera.view);
    s.SetMatrix4("model", model);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textQuadVAO);
    glCheckError();

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

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
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

void Graphics::BlitQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Shader& shader, int pass){
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

    Shader::Bind(shader);
    shader.SetFramebuffer("mainTex", *src, 0, pass);
    //src->BindColorAttachmentTexture(shader, 0);
    Graphics::DrawMeshRaw(*fullScreenQuad);
    glCheckError();
}

void Graphics::BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass){
    Assert(src != nullptr);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, src->RenderId());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst == nullptr ? 0 : dst->RenderId());
    glCheckError();

    glReadBuffer(GL_COLOR_ATTACHMENT0 + srcPass); 
    glCheckError();

    glBlitFramebuffer(0, 0, src->Width(), src->Height(), 0, 0, src->Width(), src->Height(), GL_COLOR_BUFFER_BIT, GL_NEAREST); 
    glCheckError();
}

}