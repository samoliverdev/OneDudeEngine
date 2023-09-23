#include "Renderer.h"

#include "OD/Defines.h"
#include "OD/Platform/GL.h"
#include "OD/Renderer/Shader.h"
#include "OD/Renderer/Camera.h"

namespace OD{

unsigned int lineVAO;
unsigned int lineVBO;

unsigned int wiredCubeVAO;

Ref<Shader> gismoShader;
Mesh fullScreenQuad;
Camera camera;

int Renderer::drawCalls;
int Renderer::vertices;
int Renderer::tris;

void CreateLineVAO(){
    glGenVertexArrays(1, &lineVAO);
	glBindVertexArray(lineVAO);
    glCheckError();
    
	glGenBuffers(1, &lineVBO);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
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

	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Renderer::_Initialize(){
    glEnable(GL_DEPTH_TEST); 

    fullScreenQuad = Mesh::FullScreenQuad();
    //gismoShader = Shader::CreateFromFiles("res/shaders/gizmos.vert", "res/shaders/gizmos.frag");
    gismoShader = Shader::CreateFromFile("res/Builtins/Shaders/Gizmos.glsl");
    CreateLineVAO();
    CreateWiredCubeVAO();
}

void Renderer::_Shutdown(){

}

void Renderer::Begin(){
    drawCalls = 0;
    vertices = 0;
    tris = 0;
}

void Renderer::End(){}

void Renderer::Clean(float r, float g, float b, float a){
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
}

void Renderer::SetCamera(Camera& _camera){
    camera = _camera;
}

Camera Renderer::GetCamera(){
    return camera;
}

void Renderer::DrawMeshRaw(Mesh& mesh){
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
}

void Renderer::DrawMesh(Mesh& mesh, Matrix4 modelMatrix, Shader& shader){
    Assert(mesh.IsValid() && "Mesh is not vali!");
    Assert(shader.IsValid()  && "Shader is not vali!");

    drawCalls += 1;
    vertices += mesh._vertexCount;
    tris += mesh._indiceCount;
    
    shader.Bind();
    shader.SetMatrix4("model", modelMatrix);
    shader.SetMatrix4("view", camera.view);
    shader.SetMatrix4("projection", camera.projection);

    glBindVertexArray(mesh._vao);

    if(mesh._ebo != 0){
        glDrawElements(GL_TRIANGLES, mesh._indiceCount, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, mesh._vertexCount);
    }

    glBindVertexArray(0);
    glCheckError();
}

void Renderer::DrawMesh(Mesh& mesh, Matrix4 modelMatrix, Material& material){
    material.UpdateUniforms();
    Ref<Shader> shader = material.shader;
    DrawMesh(mesh, modelMatrix, *shader);
}

void Renderer::DrawModel(Model& model, Matrix4 modelMatrix, int subMeshIndex, std::vector<Ref<Material>>* materialsOverride){
    Assert(model.meshs.size() == model.materials.size());
    Assert(subMeshIndex < (int)model.meshs.size());
    if(materialsOverride != nullptr) Assert(model.meshs.size() == materialsOverride->size());

    if(subMeshIndex > -1){
        Ref<Material> targetMaterial = subMeshIndex < (int)model.materials.size() ? model.materials[subMeshIndex] : nullptr;
        if(materialsOverride != nullptr && (*materialsOverride)[subMeshIndex] != nullptr) targetMaterial = (*materialsOverride)[subMeshIndex];
        Assert(targetMaterial != nullptr);

        targetMaterial->UpdateUniforms();
        DrawMesh(*model.meshs[subMeshIndex], modelMatrix, *targetMaterial->shader);
    } else {
        for(int i = 0; i < model.meshs.size(); i++){
            Ref<Material> targetMaterial = subMeshIndex < (int)model.materials.size() ? model.materials[i] : nullptr;
            if(materialsOverride != nullptr && (*materialsOverride)[i] != nullptr) targetMaterial = (*materialsOverride)[i];
            Assert(targetMaterial != nullptr);

            targetMaterial->UpdateUniforms();
            DrawMesh(*model.meshs[i], modelMatrix, *targetMaterial->shader);
        }
    }
}

void Renderer::DrawMeshInstancing(Mesh& mesh, Shader& shader, int count){
    Assert(mesh.IsValid() && "Mesh is not vali!");
    Assert(shader.IsValid()  && "Shader is not vali!");

    drawCalls += 1;
    vertices += mesh._vertexCount * count;
    tris += mesh._indiceCount * count;
    
    shader.Bind();
    shader.SetFloat("useInstancing", 1.0f);
    shader.SetMatrix4("view", camera.view);
    shader.SetMatrix4("projection", camera.projection);

    glBindVertexArray(mesh._vao);

    if(mesh._ebo != 0){
        glDrawElementsInstanced(GL_TRIANGLES, mesh._indiceCount, GL_UNSIGNED_INT, 0, count);
    } else {
        glDrawArraysInstanced(GL_TRIANGLES, 0, mesh._vertexCount, count);
    }

    glBindVertexArray(0);
    glCheckError();
}

void Renderer::DrawLine(Vector3 start, Vector3 end, Vector3 color, int width){
    drawCalls += 1;
    vertices += 2;
    tris += 0;

    gismoShader->Bind();
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", Matrix4::identity);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);

	glLineWidth(width);

    float line[6] = {start.x, start.y, start.z, end.x, end.y, end.z};

	glBindVertexArray(lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line), line);
	glDrawArrays(GL_LINES, 0, 2);
	glBindVertexArray(0);
    glCheckError();
}

void Renderer::DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth){
    drawCalls += 1;
    vertices += 8;
    tris += 24;

    gismoShader->Bind();
    gismoShader->SetVector3("color", color);
    gismoShader->SetMatrix4("model", modelMatrix);
    gismoShader->SetMatrix4("view", camera.view);
    gismoShader->SetMatrix4("projection", camera.projection);
    
    glLineWidth(lineWidth);
    glBindVertexArray(wiredCubeVAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glCheckError();
}

void Renderer::SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){
    glViewport(x, y, w, h);
}

void Renderer::GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h){
    GLint value[4];
    glGetIntegerv(GL_VIEWPORT, value);
    *x = value[0]; 
    *y = value[1];
    *w = value[2]; 
    *h = value[3];
}

void Renderer::SetRenderMode(RenderMode mode){
    if(mode == RenderMode::SHADED) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if(mode == RenderMode::WIREFRAME) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glCheckError();
}

void Renderer::SetDepthMask(bool value){
    if(value){
        glDepthMask(GL_TRUE);
    } else {
        glDepthMask(GL_FALSE);
    }
    glCheckError();
}

void Renderer::SetDepthTest(DepthTest depthTest){
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

void Renderer::SetCullFace(CullFace cullFace){
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

void Renderer::SetBlend(bool b){
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

void Renderer::SetBlendFunc(BlendMode sfactor, BlendMode dfactor){
    glBlendFunc(BlendModeToGL(sfactor), BlendModeToGL(dfactor));
    glCheckError();
}

void Renderer::BeginFramebuffer(Framebuffer* framebuffer){
    framebuffer->Bind();
}

void Renderer::BlitQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Shader& shader, int pass){
    Assert(src != nullptr);

    if(dst == nullptr){
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        dst->Bind();
    }
    glCheckError();
    
    Renderer::Clean(1,1,1,1);
    Renderer::SetDepthTest(DepthTest::DISABLE); 
    glCheckError();

    shader.Bind();
    shader.SetFramebuffer("mainTex", *src, 0, pass);
    //src->BindColorAttachmentTexture(shader, 0);
    Renderer::DrawMeshRaw(fullScreenQuad);
    glCheckError();
}

void Renderer::BlitFramebuffer(Framebuffer* src, Framebuffer* dst){
    Assert(src != nullptr);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, src->renderId());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst == nullptr ? 0 : dst->renderId());
    glBlitFramebuffer(0, 0, src->width(), src->height(), 0, 0, src->width(), src->height(), GL_COLOR_BUFFER_BIT, GL_NEAREST); 
}

}