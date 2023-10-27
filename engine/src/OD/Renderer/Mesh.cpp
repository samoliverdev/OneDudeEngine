#include "Mesh.h"

#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Platform/GL.h"
//#include "OD/Ultis/Obj.h"

namespace OD{

Mesh::Mesh(){
    _isReadable = true;
}

Mesh::~Mesh(){
    //Destroy();
}

void Mesh::UpdateMesh(){
    Assert(_isReadable == true && "Only can Update isReadable Mesh");

    //if(vao == 0){
        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);
        glCheckError();
    //} else {
    //    glBindVertexArray(vao);
    //}
    
    //if(vertexVbo == 0){
        glGenBuffers(1, &_vertexVbo);
        glBindBuffer(GL_ARRAY_BUFFER, _vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * vertices.size(), &vertices[0], GL_DYNAMIC_DRAW); //GL_STATIC_DRAW
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
        glCheckError();
    //} else {
    //    glBindBuffer(GL_ARRAY_BUFFER, vertexVbo);
    //    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector3) * vertices.size(), &vertices[0]);
    //}
    _vertexCount = vertices.size();

    //if(ebo == 0){
        glGenBuffers(1, &_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_DYNAMIC_DRAW);
        glCheckError();
    //} else {
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    //    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(unsigned int), &indices[0]);
    //}
    _indiceCount = indices.size();

    if(uv.size() == vertices.size()){
        //if(uvVbo == 0){
            glGenBuffers(1, &_uvVbo);
            glBindBuffer(GL_ARRAY_BUFFER, _uvVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * uv.size(), &uv[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        //} else {
        //    glBindBuffer(GL_ARRAY_BUFFER, uvVbo);
        //    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector3) * uv.size(), &uv[0]);
        //}
    }

    if(normals.size() == vertices.size()){
        //if(normalVbo == 0){
            glGenBuffers(1, &_normalVbo);
            glBindBuffer(GL_ARRAY_BUFFER, _normalVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * normals.size(), &normals[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        //} else {
        //    glBindBuffer(GL_ARRAY_BUFFER, normalVbo);
        //    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector3) * normals.size(), &normals[0]);
        //}
    }

    if(colors.size() == vertices.size()){
        //if(colorVbo == 0){
            glGenBuffers(1, &_colorVbo);
            glBindBuffer(GL_ARRAY_BUFFER, _colorVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector4) * colors.size(), &colors[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (void*)0);
            glCheckError();
        //} else {
        //    glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
        //    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector4) * colors.size(), &colors[0]);
        //}
    }

    if(tangents.size() == vertices.size()){
        //if(tangentVbo == 0){
            glGenBuffers(1, &_tangentVbo);
            glBindBuffer(GL_ARRAY_BUFFER, _tangentVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * tangents.size(), &tangents[0], GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
            glCheckError();
        //} else {
        //    glBindBuffer(GL_ARRAY_BUFFER, tangentVbo);
        //    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector3) * tangents.size(), &tangents[0]);
        //}
    }

    if(boneDatas.empty() == false) Assert(boneDatas.size() == vertices.size());
    if(boneDatas.size() == vertices.size()){
        glGenBuffers(1, &_boneDataVbo);
        glBindBuffer(GL_ARRAY_BUFFER, _boneDataVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(BoneData) * boneDatas.size(), &boneDatas[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, MAX_BONE_INFLUENCE, GL_INT, sizeof(BoneData), (void*)0);
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, sizeof(BoneData), (void*)(MAX_BONE_INFLUENCE*sizeof(int)));
        glCheckError();
    }
    
    glBindVertexArray(0);
}

void Mesh::UpdateMeshInstancingModelMatrixs(){
    Assert(_vao != 0);

    glBindVertexArray(_vao);
    glCheckError();

    if(instancingModelMatrixs.empty() == false){
        if(_instancingModelMatrixsVbo == 0){
            glGenBuffers(1, &_instancingModelMatrixsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, _instancingModelMatrixsVbo);
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
            glBindBuffer(GL_ARRAY_BUFFER, _instancingModelMatrixsVbo);
            //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Matrix4) * instancingModelMatrixs.size(), &instancingModelMatrixs[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4) * instancingModelMatrixs.size(), &instancingModelMatrixs[0], GL_DYNAMIC_DRAW);
            glCheckError();
        }
    }

    glBindVertexArray(0);
    glCheckError();
}

bool Mesh::IsValid(){
    return _vao != 0;
}

void Mesh::Destroy(){
    if(_vertexVbo != 0) glDeleteBuffers(1, &_vertexVbo);
    if(_normalVbo != 0) glDeleteBuffers(1, &_normalVbo);
    if(_uvVbo != 0) glDeleteBuffers(1, &_uvVbo);
    if(_ebo != 0) glDeleteBuffers(1, &_ebo);
    if(_vao != 0) glDeleteVertexArrays(1, &_vao);

    _vertexCount = 0;
    _vertexVbo = 0;
    _normalVbo = 0;
    _uvVbo = 0;
    _indiceCount = 0;
    _ebo = 0;
    _vao = 0;

    glCheckError();
}

Mesh Mesh::FullScreenQuad(){
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    Mesh model;
    model._isReadable = false;
    //model.ebo = 0;

    glGenVertexArrays(1, &model._vao);
    glGenBuffers(1, &model._vertexVbo);
    glBindVertexArray(model._vao);
    glBindBuffer(GL_ARRAY_BUFFER, model._vertexVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glCheckError();

    glBindVertexArray(0);
    glCheckError();

    model._vertexCount = 6;

    return model;
}

Mesh Mesh::SkyboxCube(){
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

    Mesh model;
    model._isReadable = false;

    glGenVertexArrays(1, &model._vao);
    glGenBuffers(1, &model._vertexVbo);
    glBindVertexArray(model._vao);
    glBindBuffer(GL_ARRAY_BUFFER, model._vertexVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glCheckError();

    model._vertexCount = 36;

    return model;
}

Mesh Mesh::CenterQuad(bool useIndices){
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

    Mesh model;
    model._isReadable = false;

    glGenVertexArrays(1, &model._vao);
    glBindVertexArray(model._vao);
    glCheckError();

    if(useIndices){
        glGenBuffers(1, &model._vertexVbo);
        glBindBuffer(GL_ARRAY_BUFFER, model._vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerticesIndices), quadVerticesIndices, GL_STATIC_DRAW);
        glCheckError();

        glGenBuffers(1, &model._ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model._ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW); 
        glCheckError();

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glCheckError();

        model._vertexCount = 4;
        model._indiceCount = 6;
    } else {
        glGenBuffers(1, &model._vertexVbo);
        glBindBuffer(GL_ARRAY_BUFFER, model._vertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glCheckError();

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glCheckError();

        model._vertexCount = 6;
        model._indiceCount = 0;
    }

    glBindVertexArray(0);

    return model;
}

}