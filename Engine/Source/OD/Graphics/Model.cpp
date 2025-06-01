#include "Model.h"
#include "OD/Loader/AssimpLoader.h"
#include "OD/Loader/GltfLoader2.h"
#include "OD/Loader/ObjLoader.h"
#include "OD/Core/ImGui.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/SerializationFull.h"
#include <string>
#include <fstream>

namespace OD{

Ref<Clip> Model::FindClipByName(const std::string& name){
	for(auto& i: animationClips){
		if(i->GetName() == name) return i;
	}
	return nullptr;
}

void Model::OnGui(){
	ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_Leaf);

	ImGui::Text("Render Targets Count: %zd", renderTargets.size());
	ImGui::Text("Meshs Count: %zd", meshs.size());
	ImGui::Text("Materials Count: %zd", materials.size());
	ImGui::Text("Textures Count: %zd", textures.size());
	ImGui::Text("Matrixs Count: %zd", matrixs.size());
	ImGui::Text("Animation Clips Count: %zd", animationClips.size());

	ImGui::InputFloat("LoadSettings::scale", &settings.scale);
	
	if(ImGui::Button("Apply Changes") && path != "Memory"){
		Reload();
	}
}

void Model::SetPath(const std::string& inPath){ 
	path = inPath; 
}

void Model::SetShader(Ref<Shader> shader){
	for(Ref<Material>& m: materials){
		m->SetShader(shader);
	}
}

void Model::Clear(){
	renderTargets.clear();
    meshs.clear();
    materials.clear();
    textures.clear();
    matrixs.clear();
    skeleton.Clear();
    animationClips.clear();
}

void Model::Reload(){
    if(path == "Memory"){
        LogError("Can Not Reload Texture2d From Memory");
        return;
    }

	SaveArchive(path + ".meta", settings, "settings");
    LoadFromFile(path);
}

bool Model::LoadFromFile(const std::string& path){
	if(path.empty() == false && path != "Memory") LoadOrCreateArchive(path + ".meta", settings, "settings");
    return Model::CreateFromFile(*this, path, settings);
}

std::vector<std::string> Model::GetFileAssociations(){ 
	return std::vector<std::string>{
		".obj",
		".glb",
		".gltf",
		".blend",
		".dae",
		".fbx"
	};
}

bool Model::CreateFromFile(Model& model, std::string const &path, ModelLoadSettings loadSettings){
	model.Clear();

	#ifdef USE_ASSIMP
	return AssimpLoadModel(model, path, loadSettings);
	#endif

	auto getExtension = [](const std::string& path) -> std::string {
        size_t dotPos = path.rfind('.');
        return (dotPos != std::string::npos) ? path.substr(dotPos + 1) : "";
    };

	std::string fileType = getExtension(path);

	if(fileType == "obj") return ObjLoadModel(model, path, loadSettings);
	if(fileType == "gltf") return GltfLoadModel(model, path, loadSettings);
	if(fileType == "glb") return GltfLoadModel(model, path, loadSettings);
    
	#ifdef USE_ASSIMP
	return AssimpLoadModel(model, path, loadSettings);
	#endif

	LogError("File Type Not Supported: %s", fileType.c_str());
	return false;
}

AABB Model::GenerateAABB(Model& model){
	Vector3 minAABB = Vector3(std::numeric_limits<float>::max());
	Vector3 maxAABB = Vector3(std::numeric_limits<float>::min());
	
	/*for(auto&& mesh : model.meshs){
		for(auto& vertex : mesh->vertices){
			minAABB.x = std::min(minAABB.x, vertex.x);
			minAABB.y = std::min(minAABB.y, vertex.y);
			minAABB.z = std::min(minAABB.z, vertex.z);

			maxAABB.x = std::max(maxAABB.x, vertex.x);
			maxAABB.y = std::max(maxAABB.y, vertex.y);
			maxAABB.z = std::max(maxAABB.z, vertex.z);
		}
	}*/

	for(auto i: model.renderTargets){
		auto mesh = model.meshs[i.meshIndex];
        auto targetMatrix = model.skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex); // * model.skeleton.GetInvBindPose()[i.bindPoseIndex];

		for(auto vertex : mesh->vertices){
			vertex = targetMatrix * Vector4(vertex.x, vertex.y, vertex.z, 1);

			minAABB.x = std::min(minAABB.x, vertex.x);
			minAABB.y = std::min(minAABB.y, vertex.y);
			minAABB.z = std::min(minAABB.z, vertex.z);

			maxAABB.x = std::max(maxAABB.x, vertex.x);
			maxAABB.y = std::max(maxAABB.y, vertex.y);
			maxAABB.z = std::max(maxAABB.z, vertex.z);
		}
	}

	return AABB(minAABB, maxAABB);
}

Sphere Model::GenerateSphereBV(Model& model){
	Vector3 minAABB = Vector3(std::numeric_limits<float>::max());
	Vector3 maxAABB = Vector3(std::numeric_limits<float>::min());
	for(auto&& mesh : model.meshs){
		for(auto& vertex : mesh->vertices){
			minAABB.x = std::min(minAABB.x, vertex.x);
			minAABB.y = std::min(minAABB.y, vertex.y);
			minAABB.z = std::min(minAABB.z, vertex.z);

			maxAABB.x = std::max(maxAABB.x, vertex.x);
			maxAABB.y = std::max(maxAABB.y, vertex.y);
			maxAABB.z = std::max(maxAABB.z, vertex.z);
		}
	}

	Vector3 c = (maxAABB + minAABB) * 0.5f;
	float r = (minAABB - maxAABB).length();
	return Sphere(c, r);
}

}