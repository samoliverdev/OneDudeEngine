#include "GltfLoader.h"
#include <tiny_gltf.h>
#include <string>

namespace OD{

void loadGLTFMesh(tinygltf::Model& model, tinygltf::Mesh& m, Mesh& mesh){
    const tinygltf::Mesh& gltfMesh = m; //model.meshes[0];
    const tinygltf::Primitive& primitive = gltfMesh.primitives[0];

    // Get accessor indices
    int posIndex = primitive.attributes.at("POSITION");
    int normalIndex = primitive.attributes.count("NORMAL") ? primitive.attributes.at("NORMAL") : -1;
    int uvIndex = primitive.attributes.count("TEXCOORD_0") ? primitive.attributes.at("TEXCOORD_0") : -1;
    int colorIndex = primitive.attributes.count("COLOR_0") ? primitive.attributes.at("COLOR_0") : -1;
    int tangentIndex = primitive.attributes.count("TANGENT") ? primitive.attributes.at("TANGENT") : -1;
    int weightIndex = primitive.attributes.count("WEIGHTS_0") ? primitive.attributes.at("WEIGHTS_0") : -1;
    int jointIndex = primitive.attributes.count("JOINTS_0") ? primitive.attributes.at("JOINTS_0") : -1;

    // Load vertices
    const tinygltf::Accessor& posAccessor = model.accessors[posIndex];
    const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
    const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];
    const float* posData = reinterpret_cast<const float*>(
        &posBuffer.data[posView.byteOffset + posAccessor.byteOffset]);
    
    mesh.vertices.resize(posAccessor.count);
    for (size_t i = 0; i < posAccessor.count; i++) {
        mesh.vertices[i] = Vector3(posData[i * 3], posData[i * 3 + 1], posData[i * 3 + 2]);
    }

    // Load normals if present
    if (normalIndex != -1) {
        const tinygltf::Accessor& normAccessor = model.accessors[normalIndex];
        const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
        const tinygltf::Buffer& normBuffer = model.buffers[normView.buffer];
        const float* normData = reinterpret_cast<const float*>(
            &normBuffer.data[normView.byteOffset + normAccessor.byteOffset]);
        
        mesh.normals.resize(normAccessor.count);
        for (size_t i = 0; i < normAccessor.count; i++) {
            mesh.normals[i] = Vector3(normData[i * 3], normData[i * 3 + 1], normData[i * 3 + 2]);
        }
    }

    // Load UVs if present
    if (uvIndex != -1) {
        const tinygltf::Accessor& uvAccessor = model.accessors[uvIndex];
        const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
        const tinygltf::Buffer& uvBuffer = model.buffers[uvView.buffer];
        const float* uvData = reinterpret_cast<const float*>(
            &uvBuffer.data[uvView.byteOffset + uvAccessor.byteOffset]);
        
        mesh.uv.resize(uvAccessor.count);
        for (size_t i = 0; i < uvAccessor.count; i++) {
            mesh.uv[i] = Vector3(uvData[i * 2], uvData[i * 2 + 1], 0.0f);  // UVs are 2D, using Vector3 with z=0
        }
    }

    // Load colors if present
    if (colorIndex != -1) {
        const tinygltf::Accessor& colorAccessor = model.accessors[colorIndex];
        const tinygltf::BufferView& colorView = model.bufferViews[colorAccessor.bufferView];
        const tinygltf::Buffer& colorBuffer = model.buffers[colorView.buffer];
        const float* colorData = reinterpret_cast<const float*>(
            &colorBuffer.data[colorView.byteOffset + colorAccessor.byteOffset]);
        
        mesh.colors.resize(colorAccessor.count);
        for (size_t i = 0; i < colorAccessor.count; i++) {
            mesh.colors[i] = Vector4(colorData[i * 4], colorData[i * 4 + 1], 
                                    colorData[i * 4 + 2], colorData[i * 4 + 3]);
        }
    }

    // Load indices
    const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
    const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
    const tinygltf::Buffer& indexBuffer = model.buffers[indexView.buffer];
    
    mesh.indices.resize(indexAccessor.count);
    if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        const uint16_t* indexData = reinterpret_cast<const uint16_t*>(
            &indexBuffer.data[indexView.byteOffset + indexAccessor.byteOffset]);
        for (size_t i = 0; i < indexAccessor.count; i++) {
            mesh.indices[i] = static_cast<unsigned int>(indexData[i]);
        }
    } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        const uint32_t* indexData = reinterpret_cast<const uint32_t*>(
            &indexBuffer.data[indexView.byteOffset + indexAccessor.byteOffset]);
        for (size_t i = 0; i < indexAccessor.count; i++) {
            mesh.indices[i] = indexData[i];
        }
    }

    // Load weights and joints if present (for skeletal animation)
    if (weightIndex != -1 && jointIndex != -1) {
        // Weights
        const tinygltf::Accessor& weightAccessor = model.accessors[weightIndex];
        const tinygltf::BufferView& weightView = model.bufferViews[weightAccessor.bufferView];
        const tinygltf::Buffer& weightBuffer = model.buffers[weightView.buffer];
        const float* weightData = reinterpret_cast<const float*>(
            &weightBuffer.data[weightView.byteOffset + weightAccessor.byteOffset]);
        
        mesh.weights.resize(weightAccessor.count);
        for (size_t i = 0; i < weightAccessor.count; i++) {
            mesh.weights[i] = Vector4(weightData[i * 4], weightData[i * 4 + 1],
                                    weightData[i * 4 + 2], weightData[i * 4 + 3]);
        }

        // Joints/Influences
        const tinygltf::Accessor& jointAccessor = model.accessors[jointIndex];
        const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
        const tinygltf::Buffer& jointBuffer = model.buffers[jointView.buffer];
        const uint16_t* jointData = reinterpret_cast<const uint16_t*>(
            &jointBuffer.data[jointView.byteOffset + jointAccessor.byteOffset]);
        
        mesh.influences.resize(jointAccessor.count);
        for (size_t i = 0; i < jointAccessor.count; i++) {
            mesh.influences[i] = IVector4(jointData[i * 4], jointData[i * 4 + 1],
                                        jointData[i * 4 + 2], jointData[i * 4 + 3]);
        }
    }

    // Load tangents if present
    if (tangentIndex != -1) {
        const tinygltf::Accessor& tangentAccessor = model.accessors[tangentIndex];
        const tinygltf::BufferView& tangentView = model.bufferViews[tangentAccessor.bufferView];
        const tinygltf::Buffer& tangentBuffer = model.buffers[tangentView.buffer];
        const float* tangentData = reinterpret_cast<const float*>(
            &tangentBuffer.data[tangentView.byteOffset + tangentAccessor.byteOffset]);
        
        mesh.tangents.resize(tangentAccessor.count);
        for (size_t i = 0; i < tangentAccessor.count; i++) {
            mesh.tangents[i] = Vector3(tangentData[i * 4], tangentData[i * 4 + 1], tangentData[i * 4 + 2]);
        }
    }
}

bool GltfLoadModel(
    Model& inModel, 
    std::string const &path, 
    Ref<Shader> customShader, 
    std::vector<Clip>* outClips
){
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    auto getExtension = [](const std::string& path) -> std::string {
        size_t dotPos = path.rfind('.');
        return (dotPos != std::string::npos) ? path.substr(dotPos + 1) : "";
    };

    bool ret = false;

    if(getExtension(path) == "glb"){
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.c_str());// for binary glTF(.glb)
    } else{
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.c_str());
    }

    if(!warn.empty()) printf("Warn: %s\n", warn.c_str());
    if(!err.empty()) printf("Err: %s\n", err.c_str());

    if(!ret){
        printf("Failed to parse glTF\n");
        return false;
    }

    inModel.meshs.resize(model.meshes.size());
    for(int i = 0; i < model.meshes.size(); i++){
        inModel.meshs[i] = CreateRef<Mesh>();
        loadGLTFMesh(model, model.meshes[i], *inModel.meshs[i]);
        inModel.meshs[i]->Submit();
    }

    inModel.materials.resize(1);
    inModel.materials[0] = CreateRef<Material>();

    return true;
}

}