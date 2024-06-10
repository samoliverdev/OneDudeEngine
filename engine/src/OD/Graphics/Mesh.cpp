#include "Mesh.h"

#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Platform/GL.h"
//#include "OD/Ultis/Obj.h"

namespace OD{

Mesh::Mesh(){
    isReadable = true;
}

Mesh::Mesh(const Mesh& other){
    if(other.isReadable == false){
        #ifdef GRAPHIC_LOG_ERROR
        LogError("Trying copy mesh what is not isReadable");
        #endif
        return;
    }

    isReadable = true;
    vertices = other.vertices;
    uv = other.uv;
    normals = other.normals;
    colors = other.colors;
    tangents = other.tangents;
    weights = other.weights;
	influences = other.influences;
    indices = other.indices;
    instancingModelMatrixs = other.instancingModelMatrixs;
    UpdateMesh();
    UpdateMeshInstancingModelMatrixs();
}

Mesh::~Mesh(){
    Destroy();
}

void Mesh::UpdateMesh(){
    Assert(isReadable == true && "Only can Update isReadable Mesh");

    if(vao == 0){
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glCheckError();
    } else {
        glBindVertexArray(vao);
    }
    
    if(vertexVbo == 0){
        glGenBuffers(1, &vertexVbo);
        glBindBuffer(GL_ARRAY_BUFFER, vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * vertices.size(), &vertices[0], GL_DYNAMIC_DRAW); //GL_STATIC_DRAW
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
        glCheckError();
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * vertices.size(), &vertices[0], GL_DYNAMIC_DRAW); //GL_STATIC_DRAW
    }
    vertexCount = vertices.size();

    if(indices.size() > 0){
        if(ebo == 0){
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_DYNAMIC_DRAW);
            glCheckError();
        } else {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_DYNAMIC_DRAW);
        }
    }
    indiceCount = indices.size();

    if(uv.size() == vertices.size()){
        if(uvVbo == 0){
            glGenBuffers(1, &uvVbo);
            glBindBuffer(GL_ARRAY_BUFFER, uvVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * uv.size(), &uv[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, uvVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * uv.size(), &uv[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    if(normals.size() == vertices.size()){
        if(normalVbo == 0){
            glGenBuffers(1, &normalVbo);
            glBindBuffer(GL_ARRAY_BUFFER, normalVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * normals.size(), &normals[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, normalVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * normals.size(), &normals[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    if(colors.size() == vertices.size()){
        if(colorVbo == 0){
            glGenBuffers(1, &colorVbo);
            glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * colors.size(), &colors[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * colors.size(), &colors[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    if(tangents.size() == vertices.size()){
        if(tangentVbo == 0){
            glGenBuffers(1, &tangentVbo);
            glBindBuffer(GL_ARRAY_BUFFER, tangentVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * tangents.size(), &tangents[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, tangentVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * tangents.size(), &tangents[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    if(influences.empty() == false) Assert(influences.size() == vertices.size());
    if(influences.size() == vertices.size()){
        if(jointVbo == 0){
            glGenBuffers(1, &jointVbo);
            glBindBuffer(GL_ARRAY_BUFFER, jointVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(IVector4) * influences.size(), &influences[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(5);
            glVertexAttribIPointer(5, 4, GL_INT, sizeof(IVector4), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, jointVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(IVector4) * influences.size(), &influences[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    if(weights.empty() == false) Assert(weights.size() == vertices.size());
    if(weights.size() == vertices.size()){
        if(weightsVbo == 0){
            glGenBuffers(1, &weightsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, weightsVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * weights.size(), &weights[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (void*)0);
            glCheckError();
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, weightsVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * weights.size(), &weights[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    glBindVertexArray(0);
}

void Mesh::UpdateMeshInstancingModelMatrixs(){
    Assert(vao != 0);

    glBindVertexArray(vao);
    glCheckError();

    if(instancingModelMatrixs.empty() == false){
        if(instancingModelMatrixsVbo == 0){
            glGenBuffers(1, &instancingModelMatrixsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, instancingModelMatrixsVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * instancingModelMatrixs.size(), &instancingModelMatrixs[0], GL_DYNAMIC_DRAW);
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
            glBindBuffer(GL_ARRAY_BUFFER, instancingModelMatrixsVbo);
            //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Matrix4) * instancingModelMatrixs.size(), &instancingModelMatrixs[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * instancingModelMatrixs.size(), &instancingModelMatrixs[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    glBindVertexArray(0);
    glCheckError();
}

void Mesh::UpdateMeshInstancingCustomModelMatrixs(Matrix4* modelMatrixs, int count){
    Assert(vao != 0);

    glBindVertexArray(vao);
    glCheckError();

    if(count > 0){
        if(instancingModelMatrixsVbo == 0){
            glGenBuffers(1, &instancingModelMatrixsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, instancingModelMatrixsVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * count, modelMatrixs, GL_DYNAMIC_DRAW);
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
            glBindBuffer(GL_ARRAY_BUFFER, instancingModelMatrixsVbo);
            //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Matrix4) * instancingModelMatrixs.size(), &instancingModelMatrixs[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * count, modelMatrixs, GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    glBindVertexArray(0);
    glCheckError();
}

bool Mesh::IsValid(){
    return vao != 0;
}

void Mesh::Destroy(){
    if(IsValid() == false) return;

    if(vertexVbo != 0) glDeleteBuffers(1, &vertexVbo);
    if(normalVbo != 0) glDeleteBuffers(1, &normalVbo);
    if(uvVbo != 0) glDeleteBuffers(1, &uvVbo);
    if(colorVbo != 0) glDeleteBuffers(1, &colorVbo);
    if(tangentVbo != 0) glDeleteBuffers(1, &tangentVbo);
    if(instancingModelMatrixsVbo != 0) glDeleteBuffers(1, &instancingModelMatrixsVbo);
    if(jointVbo != 0) glDeleteBuffers(1, &jointVbo);
    if(weightsVbo != 0) glDeleteBuffers(1, &weightsVbo);

    if(ebo != 0) glDeleteBuffers(1, &ebo);
    if(vao != 0) glDeleteVertexArrays(1, &vao);

    vertexCount = 0;
    vertexVbo = 0;
    normalVbo = 0;
    colorVbo = 0;
    tangentVbo = 0;
    instancingModelMatrixsVbo = 0;
    jointVbo = 0;
    weightsVbo = 0;
    uvVbo = 0;
    indiceCount = 0;
    ebo = 0;
    vao = 0;

    glCheckError();
}

Ref<Mesh> Mesh::FullScreenQuad(){
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    Ref<Mesh> mesh = CreateRef<Mesh>();
    mesh->isReadable = false;
    //model.ebo = 0;

    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vertexVbo);
    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glCheckError();

    glBindVertexArray(0);
    glCheckError();

    mesh->vertexCount = 6;

    return mesh;
}

Ref<Mesh> Mesh::SkyboxCube(){
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };

    Ref<Mesh> mesh = CreateRef<Mesh>();
    mesh->isReadable = false;

    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vertexVbo);
    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glCheckError();

    mesh->vertexCount = 36;

    return mesh;
}

Ref<Mesh> Mesh::CenterQuad(bool useIndices){
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions        // texCoords
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         1.0f, -1.0f,  0.0f, 1.0f, 0.0f,
         1.0f,  1.0f,  0.0f, 1.0f, 1.0f
    };

    float quadVerticesIndices[] = { 
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f
    };

    unsigned int quadIndices[] = {
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };

    Ref<Mesh> mesh = CreateRef<Mesh>();
    mesh->isReadable = false;

    glGenVertexArrays(1, &mesh->vao);
    glBindVertexArray(mesh->vao);
    glCheckError();

    if(useIndices){
        glGenBuffers(1, &mesh->vertexVbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerticesIndices), quadVerticesIndices, GL_STATIC_DRAW);
        glCheckError();

        glGenBuffers(1, &mesh->ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW); 
        glCheckError();

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glCheckError();

        mesh->vertexCount = 4;
        mesh->indiceCount = 6;
    } else {
        glGenBuffers(1, &mesh->vertexVbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glCheckError();

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glCheckError();

        mesh->vertexCount = 6;
        mesh->indiceCount = 0;
    }

    glBindVertexArray(0);

    return mesh;
}

}