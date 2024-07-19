#include "Model.h"
#include "OD/Loader/AssimpLoader.h"

namespace OD{

void Model::SetShader(Ref<Shader> shader){
	for(Ref<Material>& m: materials){
		m->SetShader(shader);
	}
}

bool Model::LoadFromFile(const std::string& path){
    return Model::CreateFromFile(*this, path, nullptr);
}

std::vector<std::string> Model::GetFileAssociations(){ 
	return std::vector<std::string>{
		".obj",
		".glb",
		".gltf",
		".blend",
		".dae"
	};
}

bool Model::CreateFromFile(Model& model, std::string const &path, Ref<Shader> customShader){
    return AssimpLoadModel(model, path, customShader);
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
        auto targetMatrix = model.skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex) * model.skeleton.GetInvBindPose()[i.bindPoseIndex];

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