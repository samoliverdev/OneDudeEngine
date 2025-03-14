#include "ObjLoader.h"
#include <tinyobjloader/tiny_obj_loader.h>

namespace OD{

bool ObjLoadModel(
    Model& model, 
    std::string const &path, 
    Ref<Shader> customShader, 
    std::vector<Clip>* outClips
){
    auto getDirectoryPath = [](const std::string& fullPath) -> std::string {
        // Find the last occurrence of '/' or '\'
        size_t lastSlash = fullPath.find_last_of("/\\");
        
        // If no slash found, return empty string or current directory
        if (lastSlash == std::string::npos) {
            return "./";
        }
        
        // Return substring up to and including the last slash
        return fullPath.substr(0, lastSlash + 1);
    };

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = getDirectoryPath(path); "./"; // Path to material files
    tinyobj::ObjReader reader;

    // Attempt to parse the OBJ file
    if(!reader.ParseFromFile(path, reader_config)){
        if(!reader.Error().empty()) std::cerr << "TinyObjLoader Error: " << reader.Error();
        return false;
    }

    if(!reader.Warning().empty()) std::cout << "TinyObjLoader Warning: " << reader.Warning();

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
    const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

    // Create a single mesh for now (could be split into multiple meshes if needed)
    auto mesh = std::make_shared<Mesh>();
    
    // Load vertices
    for(size_t i = 0; i < attrib.vertices.size(); i += 3){
        mesh->vertices.emplace_back(
            attrib.vertices[i],
            attrib.vertices[i + 1],
            attrib.vertices[i + 2]
        );
    }

    // Load normals
    for(size_t i = 0; i < attrib.normals.size(); i += 3){
        mesh->normals.emplace_back(
            attrib.normals[i],
            attrib.normals[i + 1],
            attrib.normals[i + 2]
        );
    }

    // Load UV coordinates
    for(size_t i = 0; i < attrib.texcoords.size(); i += 2){
        // Using Vector3 for UVs, setting z to 0
        mesh->uv.emplace_back(
            attrib.texcoords[i],
            attrib.texcoords[i + 1],
            0.0f
        );
    }

    // Load colors if available
    for(size_t i = 0; i < attrib.colors.size(); i += 3){
        mesh->colors.emplace_back(
            attrib.colors[i],
            attrib.colors[i + 1],
            attrib.colors[i + 2],
            1.0f // Assuming alpha = 1 if not provided
        );
    }

    // Process each shape
    for(const auto& shape: shapes){
        size_t index_offset = 0;
        
        // Process each face
        for(size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++){
            int fv = shape.mesh.num_face_vertices[f];
            
            // Assuming triangles (fv should be 3)
            for(size_t v = 0; v < fv; v++){
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                mesh->indices.push_back(static_cast<unsigned int>(idx.vertex_index));
            }
            index_offset += fv;

            // Create render target for this face
            Model::RenderTarget rt;
            rt.meshIndex = 0; // Only one mesh for now
            rt.materialIndex = shape.mesh.material_ids[f];
            rt.bindPoseIndex = 0;// -1; // No animation data in basic OBJ
            model.renderTargets.push_back(rt);
        }
    }

    // Add the mesh to the model
    mesh->Submit();
    model.meshs.push_back(mesh);

    // Load materials
    for(const auto& mat : materials){
        auto material = CreateRef<Material>();
        // Populate material properties here as needed
        // mat.diffuse, mat.specular, etc.
        model.materials.push_back(material);

        // Load texture if specified
        if(!mat.diffuse_texname.empty()){
            auto texture = CreateRef<Texture2D>();
            // You would need to implement texture loading logic here
            model.textures.push_back(texture);
        }
    }
    if(model.materials.empty()){
        model.materials.push_back(CreateRef<Material>());
        for(auto& rt : model.renderTargets) rt.materialIndex = 0;
    }

    Pose pose(1);
    pose.SetLocalTransform(0, Transform());
    pose.SetParent(0, -1);
    model.skeleton.Set(pose, pose, {"Root"});

    return true;
}

}