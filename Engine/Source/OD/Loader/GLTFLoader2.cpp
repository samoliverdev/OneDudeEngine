#include "GLTFLoader2.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Texture.h"
#include "OD/Graphics/Material.h"
#include <tiny_gltf.h>
#include <string>

namespace OD{

namespace GLTFHelpers {
    Transform GetLocalTransform(tinygltf::Node& node){
        Transform result;
        if(node.matrix.size() == 16) {
            std::vector<float> v;
            for(auto& i: node.matrix) v.push_back(i);
            Matrix4 mat = math::make_mat4(v.data());
            result = Transform(mat);
        } else {
            if (node.translation.size() == 3) {
                result.Position(Vector3(node.translation[0], node.translation[1], node.translation[2]));
            }
            if (node.rotation.size() == 4) {
                result.Rotation(Quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]));
            }
            if (node.scale.size() == 3) {
                result.Scale(Vector3(node.scale[0], node.scale[1], node.scale[2]));
            }
        }
        return result;
    }

    int GetNodeIndex(int childIndex, const std::vector<tinygltf::Node>& nodes){
        for(size_t i = 0; i < nodes.size(); ++i) {
            for(int child : nodes[i].children) {
                if(child == childIndex) return static_cast<int>(i);
            }
        }
        return -1;
    }

    void GetScalarValues(tinygltf::Model& model, std::vector<float>& outScalars, unsigned int inComponentCount, tinygltf::Accessor& accessor){
        outScalars.resize(accessor.count * inComponentCount);
        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[view.buffer];
        const unsigned char* baseData = &buffer.data[view.byteOffset + accessor.byteOffset];
        size_t stride = view.byteStride ? view.byteStride : (inComponentCount * sizeof(float));
        if(accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
            Assert(false && "Only FLOAT component type supported");
        }
        for(size_t i = 0; i < accessor.count; ++i) {
            const float* floatData = reinterpret_cast<const float*>(baseData + i * stride);
            for(size_t j = 0; j < inComponentCount; ++j) {
                outScalars[i * inComponentCount + j] = floatData[j];
            }
        }
    }

    template<typename T, int N>
    void TrackFromChannel(tinygltf::Model& model, Track<T, N>& inOutTrack, tinygltf::Animation& anim, const tinygltf::AnimationChannel& channel){
        auto& sampler = anim.samplers[channel.sampler];
        Interpolation interpolation = Interpolation::Constant;
        if (sampler.interpolation == "LINEAR") interpolation = Interpolation::Linear;
        else if (sampler.interpolation == "CUBICSPLINE") interpolation = Interpolation::Cubic;
        bool isSamplerCubic = interpolation == Interpolation::Cubic;
        inOutTrack.SetInterpolation(interpolation);

        std::vector<float> timelineFloats;
        GetScalarValues(model, timelineFloats, 1, model.accessors[sampler.input]);
        std::vector<float> valueFloats;
        GetScalarValues(model, valueFloats, N * (isSamplerCubic ? 3 : 1), model.accessors[sampler.output]);

        unsigned int numFrames = model.accessors[sampler.input].count;
        inOutTrack.Resize(numFrames);
        for(unsigned int i = 0; i < numFrames; ++i){
            int baseIndex = i * N * (isSamplerCubic ? 3 : 1);
            Frame<N>& frame = inOutTrack[i];
            frame.time = timelineFloats[i];
            int offset = 0;
            for(int j = 0; j < N; ++j) frame.in[j] = isSamplerCubic ? valueFloats[baseIndex + offset++] : 0.0f;
            for(int j = 0; j < N; ++j) frame.value[j] = valueFloats[baseIndex + offset++];
            for(int j = 0; j < N; ++j) frame.out[j] = isSamplerCubic ? valueFloats[baseIndex + offset++] : 0.0f;
        }
    }

    void MeshFromAttribute(tinygltf::Model& model, Mesh& outMesh, tinygltf::Primitive& primitive, const std::string& attribName, const tinygltf::Skin* skin = nullptr, const std::vector<tinygltf::Node>* nodes = nullptr) {
        if(!primitive.attributes.count(attribName)) return;
        int accessorIdx = primitive.attributes.at(attribName);
        tinygltf::Accessor& accessor = model.accessors[accessorIdx];
        tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        const unsigned char* baseData = &model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset];
        size_t stride = view.byteStride ? view.byteStride : 0;

        unsigned int componentCount = 0;
        if(accessor.type == TINYGLTF_TYPE_VEC2) componentCount = 2;
        else if(accessor.type == TINYGLTF_TYPE_VEC3) componentCount = 3;
        else if(accessor.type == TINYGLTF_TYPE_VEC4) componentCount = 4;

        if(attribName == "POSITION"){
            stride = stride ? stride : (3 * sizeof(float));
            outMesh.vertices.resize(accessor.count);
            for(size_t i = 0; i < accessor.count; ++i) {
                const float* data = reinterpret_cast<const float*>(baseData + i * stride);
                outMesh.vertices[i] = Vector3(data[0], data[1], data[2]);
            }
        }
        if(attribName == "NORMAL"){
            stride = stride ? stride : (3 * sizeof(float));
            outMesh.normals.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; ++i) {
                const float* data = reinterpret_cast<const float*>(baseData + i * stride);
                Vector3 normal(data[0], data[1], data[2]);
                outMesh.normals[i] = glm::length2(normal) < 0.000001f ? Vector3(0, 1, 0) : glm::normalize(normal);
            }
        }
        if(attribName == "TANGENT"){
            stride = stride ? stride : (3 * sizeof(float));
            outMesh.tangents.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; ++i) {
                const float* data = reinterpret_cast<const float*>(baseData + i * stride);
                Vector3 tangents(data[0], data[1], data[2]);
                outMesh.tangents[i] = tangents;
            }
        }
        if(attribName == "TEXCOORD_0"){
            stride = stride ? stride : (2 * sizeof(float));
            outMesh.uv.resize(accessor.count);
            for(size_t i = 0; i < accessor.count; ++i){
                const float* data = reinterpret_cast<const float*>(baseData + i * stride);
                outMesh.uv[i] = Vector3(data[0], data[1], 0);
            }
        }
        if(attribName == "COLOR_0"){
            stride = stride ? stride : (4 * sizeof(float));
            outMesh.colors.resize(accessor.count);
            for(size_t i = 0; i < accessor.count; ++i){
                const float* data = reinterpret_cast<const float*>(baseData + i * stride);
                outMesh.colors[i] = Vector4(data[0], data[1], data[2], data[3]);
            }
        }
        if(attribName == "WEIGHTS_0"){
            stride = stride ? stride : (4 * sizeof(float));
            outMesh.weights.resize(accessor.count);
            for(size_t i = 0; i < accessor.count; ++i){
                const float* data = reinterpret_cast<const float*>(baseData + i * stride);
                outMesh.weights[i] = Vector4(data[0], data[1], data[2], data[3]);
            }
        }
        if(attribName == "JOINTS_0" && skin && nodes){
            outMesh.influences.resize(accessor.count);
            if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT){
                stride = stride ? stride : (4 * sizeof(uint16_t));
                for(size_t i = 0; i < accessor.count; ++i){
                    const uint16_t* data = reinterpret_cast<const uint16_t*>(baseData + i * stride);
                    IVector4 joints(data[0], data[1], data[2], data[3]);
                    joints.x = std::max(0, skin->joints[joints.x]); // Map skin-relative to node index
                    joints.y = std::max(0, skin->joints[joints.y]);
                    joints.z = std::max(0, skin->joints[joints.z]);
                    joints.w = std::max(0, skin->joints[joints.w]);
                    outMesh.influences[i] = joints;
                }
            } else if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE){
                stride = stride ? stride : (4 * sizeof(uint8_t));
                for(size_t i = 0; i < accessor.count; ++i) {
                    const uint8_t* data = reinterpret_cast<const uint8_t*>(baseData + i * stride);
                    IVector4 joints(data[0], data[1], data[2], data[3]);
                    joints.x = std::max(0, skin->joints[joints.x]);
                    joints.y = std::max(0, skin->joints[joints.y]);
                    joints.z = std::max(0, skin->joints[joints.z]);
                    joints.w = std::max(0, skin->joints[joints.w]);
                    outMesh.influences[i] = joints;
                }
            }
        }
    }
} // namespace GLTFHelpers

Pose LoadRestPose(tinygltf::Model& model){
    int boneCount = model.nodes.size();
    Pose result(boneCount);

    for(int i = 0; i < boneCount; ++i) {
        auto& node = model.nodes[i];
        Transform transform = GLTFHelpers::GetLocalTransform(node);
        result.SetLocalTransform(i, transform);
        int parent = GLTFHelpers::GetNodeIndex(i, model.nodes);
        result.SetParent(i, parent);
    }
    return result;
}

std::vector<std::string> LoadJointNames(tinygltf::Model& model){
    std::vector<std::string> result(model.nodes.size(), "Not Set");
    for (size_t i = 0; i < model.nodes.size(); ++i) {
        result[i] = model.nodes[i].name.empty() ? "EMPTY NODE" : model.nodes[i].name;
    }
    return result;
}

std::vector<Ref<Clip>> LoadAnimationClips(tinygltf::Model& model){
    std::vector<Ref<Clip>> result(model.animations.size());
    for(size_t i = 0; i < model.animations.size(); ++i){
        result[i] = CreateRef<Clip>();
        result[i]->SetName(model.animations[i].name.empty() ? "Clip " + std::to_string(i) : model.animations[i].name);
        for(const auto& channel : model.animations[i].channels){
            int nodeId = channel.target_node;
            if(nodeId == -1) continue;
            if(channel.target_path == "translation"){
                VectorTrack& track = (*result[i])[nodeId].GetPositionTrack();
                GLTFHelpers::TrackFromChannel<Vector3, 3>(model, track, model.animations[i], channel);
            } else if(channel.target_path == "rotation"){
                QuaternionTrack& track = (*result[i])[nodeId].GetRotationTrack();
                GLTFHelpers::TrackFromChannel<Quaternion, 4>(model, track, model.animations[i], channel);
            } else if(channel.target_path == "scale"){
                VectorTrack& track = (*result[i])[nodeId].GetScaleTrack();
                GLTFHelpers::TrackFromChannel<Vector3, 3>(model, track, model.animations[i], channel);
            }
        }
        result[i]->RecalculateDuration();
    }
    return result;
}

Pose LoadBindPose(tinygltf::Model& model) {
    Pose restPose = LoadRestPose(model);
    unsigned int numBones = restPose.Size();
    std::vector<Transform> worldBindPose(numBones);
    for (unsigned int i = 0; i < numBones; ++i) {
        worldBindPose[i] = restPose.GetGlobalTransform(i);
    }

    // Overwrite bind poses for skinned joints
    for (const auto& skin : model.skins) {
        std::vector<float> invBindAccessor;
        GLTFHelpers::GetScalarValues(model, invBindAccessor, 16, model.accessors[skin.inverseBindMatrices]);
        for (size_t j = 0; j < skin.joints.size(); ++j) {
            float* matrix = &invBindAccessor[j * 16];
            Matrix4 invBindMatrix = math::make_mat4(matrix);
            Matrix4 bindMatrix = math::inverse(invBindMatrix);
            Transform bindTransform = Transform(bindMatrix);
            int jointIndex = skin.joints[j]; // Direct node index since all nodes are in Pose
            worldBindPose[jointIndex] = bindTransform;
        }
    }

    Pose bindPose = restPose;
    for (unsigned int i = 0; i < numBones; i++) {
        Transform current = worldBindPose[i];
        int p = bindPose.GetParent(i);
        if (p >= 0) {
            Transform parent = worldBindPose[p];
            Transform temp = Transform::Inverse(parent);
            current = Transform::Combine(temp, current);
        }
        bindPose.SetLocalTransform(i, current);
    }
    return bindPose;
}

Skeleton LoadSkeleton(tinygltf::Model& model) {
    return Skeleton(LoadRestPose(model), LoadBindPose(model), LoadJointNames(model));
}

std::vector<Ref<Mesh>> LoadMeshes(tinygltf::Model& model) {
    std::vector<Ref<Mesh>> result;
    for(const auto& node : model.nodes){
        if(node.mesh < 0) continue;
        auto& gltfMesh = model.meshes[node.mesh];
        const tinygltf::Skin* skin = (node.skin >= 0) ? &model.skins[node.skin] : nullptr;
        for(size_t j = 0; j < gltfMesh.primitives.size(); ++j){
            auto mesh = CreateRef<Mesh>();
            auto& primitive = gltfMesh.primitives[j];

            GLTFHelpers::MeshFromAttribute(model, *mesh, primitive, "POSITION");
            if(primitive.attributes.count("NORMAL")) GLTFHelpers::MeshFromAttribute(model, *mesh, primitive, "NORMAL");
            if(primitive.attributes.count("TANGENT")) GLTFHelpers::MeshFromAttribute(model, *mesh, primitive, "TANGENT");
            if(primitive.attributes.count("TEXCOORD_0")) GLTFHelpers::MeshFromAttribute(model, *mesh, primitive, "TEXCOORD_0");
            if(primitive.attributes.count("NORMAL")) GLTFHelpers::MeshFromAttribute(model, *mesh, primitive, "NORMAL");
            if(primitive.attributes.count("COLOR_0")) GLTFHelpers::MeshFromAttribute(model, *mesh, primitive, "COLOR_0");
            if(primitive.attributes.count("WEIGHTS_0")) GLTFHelpers::MeshFromAttribute(model, *mesh, primitive, "WEIGHTS_0");
            if(primitive.attributes.count("JOINTS_0")) GLTFHelpers::MeshFromAttribute(model, *mesh, primitive, "JOINTS_0", skin, &model.nodes);

            if(primitive.indices >= 0){
                tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
                const unsigned char* indexData = &model.buffers[indexView.buffer].data[indexView.byteOffset + indexAccessor.byteOffset];
                size_t stride = indexView.byteStride ? indexView.byteStride : (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? sizeof(uint16_t) : sizeof(uint32_t));
                mesh->indices.resize(indexAccessor.count);
                if(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT){
                    for(size_t k = 0; k < indexAccessor.count; ++k) {
                        mesh->indices[k] = reinterpret_cast<const uint16_t*>(indexData + k * stride)[0];
                    }
                } else if(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT){
                    for(size_t k = 0; k < indexAccessor.count; ++k) {
                        mesh->indices[k] = reinterpret_cast<const uint32_t*>(indexData + k * stride)[0];
                    }
                }
            }
            mesh->Submit();
            result.push_back(mesh);
        }
    }
    return result;
}

bool GltfLoadModel(
    Model& model, 
    std::string const &path, 
    ModelLoadSettings loadSettings, 
    std::vector<Clip>* outClips
){
    tinygltf::TinyGLTF loader;
    tinygltf::Model gltfModel;
    std::string err, warn;
    bool ret = false;
    if(std::string(path).substr(std::string(path).find_last_of(".") + 1) == "glb"){
        ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);
    } else {
        ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path);
    }
    if(!warn.empty()) std::cout << "Warn: " << warn << std::endl;
    if(!err.empty()) std::cout << "Err: " << err << std::endl;
    if(!ret){
        printf("Failed to parse glTF\n");
        return false;
    }

    // Load meshes and set render targets
    model.meshs = LoadMeshes(gltfModel);
    model.renderTargets.resize(model.meshs.size());
    for(size_t i = 0; i < model.meshs.size(); ++i){
        int materialIndex = -1;
        size_t meshIndex = 0;
        for(const auto& gltfMesh : gltfModel.meshes){
            for(size_t j = 0; j < gltfMesh.primitives.size(); ++j){
                if(meshIndex++ == i){
                    materialIndex = gltfMesh.primitives[j].material;
                    break;
                }
            }
            if(materialIndex >= 0) break;
        }
        model.renderTargets[i] = {static_cast<int>(i), materialIndex >= 0 ? materialIndex : 0, 0};
    }

    for(auto& i: gltfModel.images){
        auto tex = Texture2D::CreateFromRaw(
            i.image.data(),
            //i.image.size(),
            i.width,
            i.height,
            TextureDataType::UnsignedByte,
            Texture2DSetting{TextureFilter::Linear, TextureWrapping::Repeat, true, i.component == 4 ? TextureFormat::RGBA : TextureFormat::RGB}
        );
        model.textures.push_back(tex);
    }

    model.materials.resize(gltfModel.materials.size());
    for(size_t i = 0; i < gltfModel.materials.size(); ++i){
        auto mat = CreateRef<Material>();
        auto& gltfMat = gltfModel.materials[i];
        
        if(gltfMat.pbrMetallicRoughness.baseColorTexture.index >= 0){
            mat->SetTexture("mainTex", model.textures[gltfMat.pbrMetallicRoughness.baseColorTexture.index]);
        }
        auto& baseColorFactor = gltfMat.pbrMetallicRoughness.baseColorFactor;
        mat->SetVector4("color", Vector4(baseColorFactor[0], baseColorFactor[1], baseColorFactor[2], baseColorFactor[3]));

        // Metallic-Roughness
        //if(gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
        //    mat->metallicRoughnessTextureIndex = gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index;
        //}
        //mat->metallicFactor = gltfMat.pbrMetallicRoughness.metallicFactor;
        //mat->roughnessFactor = gltfMat.pbrMetallicRoughness.roughnessFactor;

        // Normal
        if(gltfMat.normalTexture.index >= 0){
            mat->SetTexture("normal", model.textures[gltfMat.normalTexture.index]);
        }

        // Occlusion
        //if(gltfMat.occlusionTexture.index >= 0) {
        //    mat->occlusionTextureIndex = gltfMat.occlusionTexture.index;
        //}
        // Emissive
        //if(gltfMat.emissiveTexture.index >= 0) {
        //    mat->emissiveTextureIndex = gltfMat.emissiveTexture.index;
        //}
        //zuto& emissiveFactor = gltfMat.emissiveFactor;
        //mat->emissiveFactor = Vector3(emissiveFactor[0], emissiveFactor[1], emissiveFactor[2]);

        model.materials[i] = mat;
    }
    if(model.materials.empty()){
        model.materials.push_back(CreateRef<Material>());
        for(auto& rt : model.renderTargets) rt.materialIndex = 0;
    }

    model.skeleton = LoadSkeleton(gltfModel);
    //model.animationClips = LoadAnimationClips(gltfModel);

    // Set shader and path
    if(loadSettings.customShader) model.SetShader(loadSettings.customShader);
    model.SetPath(path);

    return true;
}

}
