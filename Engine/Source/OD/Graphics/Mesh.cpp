#include "OD/pch.h"
#include "Mesh.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Core/ImGui.h"
#include "OD/Platform/OpenGL/GL.h"
#include "OD/Serialization/SerializationFull.h"
#include "OD/Core/FileSystemUltis.h"

namespace OD{

extern GraphicsDevice* graphicsDevice;

IdPool meshIdPool;

Mesh::Mesh(){
    isReadable = true;
    id = meshIdPool.Pop();
}

Mesh::Mesh(const std::string& label){
    path = "#" + label;
    isReadable = true;
    id = meshIdPool.Pop();
}

Mesh::Mesh(const Mesh& other){
    id = meshIdPool.Pop();

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
    Submit();
    SubmitInstancingModelMatrixs();
}

Mesh::~Mesh(){
    meshIdPool.Push(id);
    graphicsDevice->MeshDestroy(*this);
}

void Mesh::AppedFrom(const Mesh& mesh){
    unsigned int vertexOffset = static_cast<unsigned int>(vertices.size());

    vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
    uv.insert(uv.end(), mesh.uv.begin(), mesh.uv.end());
    normals.insert(normals.end(), mesh.normals.begin(), mesh.normals.end());
    colors.insert(colors.end(), mesh.colors.begin(), mesh.colors.end());
    tangents.insert(tangents.end(), mesh.tangents.begin(), mesh.tangents.end());
    weights.insert(weights.end(), mesh.weights.begin(), mesh.weights.end());
    influences.insert(influences.end(), mesh.influences.begin(), mesh.influences.end());
    //indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());

    for(unsigned int index : mesh.indices){
        indices.push_back(index + vertexOffset);
    }

    for(unsigned int idx : mesh.indices){
        Assert(idx < mesh.vertices.size());
    }

    unsigned maxI = 0;
    for(auto i : indices) maxI = std::max(maxI, i);
    LogInfo("Mesh max index = {}", maxI);
}

void Mesh::OnGui(){
    ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_Leaf);

    ImGui::Text("Vertex Count %d", vertexCount);
    ImGui::Text("Indice Count %d", indiceCount);
    ImGui::Spacing();ImGui::Spacing();
    ImGui::Text("Runtime Vertices Count %zd", vertices.size());
    ImGui::Text("Runtime UV Count %zd", uv.size());
    ImGui::Text("Runtime Colors Count %zd", colors.size());
    ImGui::Text("Runtime Tangents Count %zd", tangents.size());
    ImGui::Text("Runtime Weights Count %zd", weights.size());
    ImGui::Text("Runtime Influences Count %zd", influences.size());
    ImGui::Text("Runtime Indices Count %zd", indices.size());
}

//Source: https://gamedev.stackexchange.com/questions/152991/how-can-i-calculate-normals-using-a-vertex-and-index-buffer
void Mesh::CalculateNormals(){
    normals.resize(vertices.size());
    //tangents.resize(vertices.size());

    // Zero-out our normal buffer to start from a clean slate.
    /*for(int vertex = 0; vertex < vertices.size(); vertex++){
        normals[vertex] = Vector3Zero;
    }*/

    // For each face, compute the face normal, and accumulate it into each vertex.
    for(int index = 0; index < indices.size(); index += 3) {
        int vertexA = indices[index];
        int vertexB = indices[index + 1];
        int vertexC = indices[index + 2];    

        auto edgeAB = vertices[vertexB] - vertices[vertexA];
        auto edgeAC = vertices[vertexC] - vertices[vertexA];

        // The cross product is perpendicular to both input vectors (normal to the plane).
        // Flip the argument order if you need the opposite winding.    
        auto areaWeightedNormal = math::cross(edgeAB, edgeAC);

        // Don't normalize this vector just yet. Its magnitude is proportional to the
        // area of the triangle (times 2), so this helps ensure tiny/skinny triangles
        // don't have an outsized impact on the final normal per vertex.

        // Accumulate this cross product into each vertex normal slot.
        normals[vertexA] += areaWeightedNormal;
        normals[vertexB] += areaWeightedNormal;
        normals[vertexC] += areaWeightedNormal;
    }       

    // Finally, normalize all the sums to get a unit-length, area-weighted average.
    for(int vertex = 0; vertex < vertices.size(); vertex++){  
        normals[vertex] = math::normalize(normals[vertex]);
    }
}

void Mesh::CalculateTangent(){
    tangents.resize(vertices.size());
    Assert(vertices.size() == uv.size());

    //iterate the indices array
    for(size_t i = 0; i < indices.size(); i+=3){ //we need to handle 3 vertices --> one triangle
        //calculate indices
        unsigned int i1 = indices[i];
        unsigned int i2 = indices[i + 1];
        unsigned int i3 = indices[i + 2];

        glm::vec3 edge1 = glm::vec3(vertices[i2].x, vertices[i2].y, vertices[i2].z) - glm::vec3(vertices[i1].x, vertices[i1].y, vertices[i1].z);
        glm::vec3 edge2 = glm::vec3(vertices[i3].x, vertices[i3].y, vertices[i3].z) - glm::vec3(vertices[i1].x, vertices[i1].y, vertices[i1].z);
        glm::vec2 deltaUV1 = glm::vec2(uv[i2].x, uv[i2].y) - glm::vec2(uv[i1].x, uv[i1].y);
        glm::vec2 deltaUV2 = glm::vec2(uv[i3].x, uv[i3].y) - glm::vec2(uv[i1].x, uv[i1].y);

        // calculate tangent.
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);

        // calculate bitangent.
        glm::vec3 bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

        tangents[i1] = tangent;
        tangents[i2] = tangent;
        tangents[i3] = tangent;
    }
}

size_t Mesh::CalculateRamUsage(){
    size_t ram = 0;

    ram += vertices.size() * sizeof(Vector3);
    ram += uv.size() * sizeof(Vector3);
    ram += normals.size() * sizeof(Vector3);
    ram += colors.size() * sizeof(Vector4);
    ram += tangents.size() * sizeof(Vector3);
    ram += weights.size() * sizeof(Vector4);
    ram += influences.size() * sizeof(IVector4);
    ram += indices.size() * sizeof(unsigned int);
    ram += instancingModelMatrixs.size() * sizeof(Matrix4);

    return ram;
}

size_t Mesh::CalculateVRamUsage(){
    return CalculateRamUsage();//For now is the same
}

void Mesh::Submit(){
    Assert(isReadable == true && "Only can Update isReadable Mesh");
    Submit(
        &indices,
        &vertices,
        &uv,
        &normals,
        &colors,
        &tangents,
        &weights,
        &influences
    );
}

void Mesh::Submit(
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
    graphicsDevice->MeshCreateOrSubmit(*this, indices, vertices, uv, normals, colors, tangents, weights, influences);
}

void Mesh::SubmitInstancingModelMatrixs(){
    graphicsDevice->MeshSubmitInstancingModelMatrixs(*this);
}

void Mesh::SubmitInstancingCustomModelMatrixs(Matrix4* modelMatrixs, int count){
    graphicsDevice->MeshSubmitInstancingCustomModelMatrixs(*this, modelMatrixs, count);
}

Ref<Mesh> Mesh::FullScreenQuad(){
    Ref<Mesh> mesh = CreateRef<Mesh>();
    //mesh->isReadable = false;
    mesh->vertices = {
        // positions   // texCoords
        Vector3(-1.0f,  1.0f,  0),
        Vector3(-1.0f, -1.0f,  0),
        Vector3( 1.0f, -1.0f,  0),
        Vector3(-1.0f,  1.0f,  0),
        Vector3( 1.0f, -1.0f,  0),
        Vector3( 1.0f,  1.0f,  0)
    };
    mesh->uv = {
        Vector3(0, 1, 0),
        Vector3(0, 0, 0),
        Vector3(1, 0, 0),
        Vector3(0, 1, 0),
        Vector3(1, 0, 0),
        Vector3(1, 1, 0)
    };
    mesh->indices = {
        0, 1, 2,
        3, 4, 5
    };
    mesh->Submit();
    return mesh;

    /*
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

    #ifdef USE_VAO
    glGenVertexArrays(1, &mesh->vao);
    glBindVertexArray(mesh->vao);
    #endif

    glGenBuffers(1, &mesh->vertexVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glCheckError();

    #ifdef USE_VAO
    glBindVertexArray(0);
    glCheckError();
    #endif

    mesh->vertexCount = 6;

    return mesh;*/
}

Ref<Mesh> Mesh::SkyboxCube(){
    std::vector<Vector3> pos = {
        Vector3(-1.0f,  1.0f, -1.0f),
        Vector3(-1.0f, -1.0f, -1.0f),
        Vector3(1.0f, -1.0f, -1.0f),
        Vector3(1.0f, -1.0f, -1.0f),
        Vector3(1.0f,  1.0f, -1.0f),
        Vector3(-1.0f,  1.0f, -1.0f),

        Vector3(-1.0f, -1.0f,  1.0f),
        Vector3(-1.0f, -1.0f, -1.0f),
        Vector3(-1.0f,  1.0f, -1.0f),
        Vector3(-1.0f,  1.0f, -1.0f),
        Vector3(-1.0f,  1.0f,  1.0f),
        Vector3(-1.0f, -1.0f,  1.0f),

        Vector3(1.0f, -1.0f, -1.0f),
        Vector3(1.0f, -1.0f,  1.0f),
        Vector3(1.0f,  1.0f,  1.0f),
        Vector3(1.0f,  1.0f,  1.0f),
        Vector3(1.0f,  1.0f, -1.0f),
        Vector3(1.0f, -1.0f, -1.0f),

        Vector3(-1.0f, -1.0f,  1.0f),
        Vector3(-1.0f,  1.0f,  1.0f),
        Vector3(1.0f,  1.0f,  1.0f),
        Vector3(1.0f,  1.0f,  1.0f),
        Vector3(1.0f, -1.0f,  1.0f),
        Vector3(-1.0f, -1.0f,  1.0f),

        Vector3(-1.0f,  1.0f, -1.0f),
        Vector3(1.0f,  1.0f, -1.0f),
        Vector3(1.0f,  1.0f,  1.0f),
        Vector3(1.0f,  1.0f,  1.0f),
        Vector3(-1.0f,  1.0f,  1.0f),
        Vector3(-1.0f,  1.0f, -1.0f),

        Vector3(-1.0f, -1.0f, -1.0f),
        Vector3(-1.0f, -1.0f,  1.0f),
        Vector3(1.0f, -1.0f, -1.0f),
        Vector3(1.0f, -1.0f, -1.0f),
        Vector3(-1.0f, -1.0f,  1.0f),
        Vector3(1.0f, -1.0f,  1.0f)
    };  
    Ref<Mesh> mesh = CreateRef<Mesh>();
    mesh->isReadable = false;
    mesh->Submit(nullptr, &pos);
    return mesh;

    /*float skyboxVertices[] = {
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
    mesh->isReadable = false;*/

    //Assert(false);

    /*#ifdef USE_VAO
    glGenVertexArrays(1, &mesh->vao);
    glBindVertexArray(mesh->vao);
    #endif

    glGenBuffers(1, &mesh->vertexVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glCheckError();

    mesh->vertexCount = 36;*/

    //return mesh;
}

Ref<Mesh> Mesh::CenterQuad(bool useIndices){
    Ref<Mesh> mesh = CreateRef<Mesh>();
    mesh->isReadable = false;
    std::vector<Vector3> pos = {
        Vector3(-0.5f,  0.5f, 0.0f),
        Vector3(-0.5f, -0.5f, 0.0f),
        Vector3( 0.5f, -0.5f, 0.0f),
        Vector3(-0.5f,  0.5f, 0.0f),
        Vector3( 0.5f, -0.5f, 0.0f),
        Vector3( 0.5f,  0.5f, 0.0f),
    };
    std::vector<Vector3> uv = {
        Vector3( 0.0f, 1.0f, 0.0f),
        Vector3( 0.0f, 0.0f, 0.0f),
        Vector3( 1.0f, 0.0f, 0.0f),
        Vector3( 0.0f, 1.0f, 0.0f),
        Vector3( 1.0f, 0.0f, 0.0f),
        Vector3( 1.0f, 1.0f, 0.0f),
    };
    mesh->Submit(nullptr, &pos, &uv);
    return mesh;

    /*
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions        // texCoords
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f,

        -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.0f, 1.0f, 1.0f
    };

    float quadVerticesIndices[] = { 
        0.5f,  0.5f, 0.0f, 0.5f, 0.5f,
        0.5f, -0.5f, 0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 0.5f
    };

    unsigned int quadIndices[] = {
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };

    Ref<Mesh> mesh = CreateRef<Mesh>();
    mesh->isReadable = false;

    #ifdef USE_VAO
    glGenVertexArrays(1, &mesh->vao);
    glBindVertexArray(mesh->vao);
    glCheckError();
    #endif

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

    #ifdef USE_VAO
    glBindVertexArray(0);
    #endif

    return mesh;*/
}

bool Mesh::LoadFromFile(const std::string& inpath){
    std::ifstream stream(inpath, std::ios::binary);
    if(stream.is_open() == false) return false;

    auto extension = GetFileExtension(inpath);

    if(extension == "meshasset"){
        cereal::PortableBinaryInputArchive ar(stream);
        ar(*this);//ArchiveDump(ar, *this);
        return true;
    }

    if(extension == "meshbin"){
        cereal::BinaryInputArchive ar(stream);
        ar(*this);//ArchiveDump(ar, *this);
        return true;
    }

    return false;
}

bool Mesh::Save(const std::string& outPath, SaveType type){
    if(type == Resource::SaveType::SettingOnly) return false;

    std::ofstream os(outPath, std::ios::binary);
    Assert(os.is_open());

    if(type == Resource::SaveType::AssetBinary){
        cereal::PortableBinaryOutputArchive ar(os);
        ar(*this);
    }
    if(type == Resource::SaveType::FinalBinary){
        cereal::BinaryOutputArchive ar(os);
        ar(*this);
    }

    return true;
}

}