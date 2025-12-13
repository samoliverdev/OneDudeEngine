#include "OD/pch.h"
#include "Model.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "Framebuffer.h"
#include "OD/Loader/AssimpLoader.h"
#include "OD/Loader/GltfLoader2.h"
#include "OD/Loader/ObjLoader.h"
#include "OD/Core/ImGui.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/SerializationFull.h"
#include "OD/Physics/PhysicsSystem.h"
#include "OD/Core/Application.h"
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include "OD/Editor/Editor.h"

namespace OD{

Ref<ClipT> Model::FindClipByName(const std::string& name){
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

	ImGui::Checkbox("LoadSettings::useOnlySkinnedBones", &settings.useOnlySkinnedBones);
	ImGui::Checkbox("LoadSettings::generateColliderData", &settings.generateColliderData);
	
	if(ImGui::Button("Apply Changes") && PathIsValid()){
		Reload();
	}

	if(PathIsValid()){
		auto* editor = Application::GetModuleByType<Editor>();
		auto* framebuffer = editor->AssetPreviewFramebuffer();
		editor->SetModelAssetPreview(path);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		float aspect = framebuffer->Width() / framebuffer->Height();
		ImGui::Image(framebuffer->ColorAttachmentId(0), ImVec2(viewportPanelSize.x, viewportPanelSize.x * aspect), ImVec2(0, 1), ImVec2(1, 0));
	} else {
		ImGui::Text("Can not preview this model!!!");
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
    if(PathIsValid() == false){
        LogError("Can Not Reload Texture2d From Memory");
        return;
    }

	SaveArchive(path + ".meta", settings, "settings");
    LoadFromFile(path);
}

bool Model::LoadFromFile(const std::string& path){
	//if(path.empty() == false && path != "#Memory") 
		LoadOrCreateArchive(path + ".meta", settings, "settings");
    
	//return Model::CreateFromFile(*this, path, settings);

	bool r = Model::CreateFromFile(*this, path, settings);
	if(r == false) return false;

	if(settings.generateColliderData){
		modelShapeData = CreateMeshShapeData(*this);
	}

	return true;
}

bool Model::LoadFromPackage(const std::string& path, Package& package){
	bool r = Model::CreateFromPackage(*this, path, package, settings);
	if(r == false) return false;
	return true;
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

	auto getExtension = [](const std::string& path) -> std::string {
        size_t dotPos = path.rfind('.');
        return (dotPos != std::string::npos) ? path.substr(dotPos + 1) : "";
    };

	std::string fileType = getExtension(path);

	if(fileType == "modelasset"){
		Assert(false);
		/*std::ifstream stream(path, std::ios::binary);
		if(stream.is_open() == false) return false;

		cereal::PortableBinaryInputArchive ar(stream);
		ar(model);//ArchiveDump(ar, *this);

		model.SetPath(path);
		return true;*/
	}

	if(fileType == "modelbin"){
		std::ifstream stream(path, std::ios::binary);
		if(stream.is_open() == false) return false;

		cereal::BinaryInputArchive ar(stream);
		model.LoadFrom(ar);
		model.SetPath(path);
		return true;
	}

	#ifdef USE_ASSIMP
	return AssimpLoadModel(model, path, loadSettings);
	#endif

	if(fileType == "obj") return ObjLoadModel(model, path, loadSettings);
	if(fileType == "gltf") return GltfLoadModel(model, path, loadSettings);
	if(fileType == "glb") return GltfLoadModel(model, path, loadSettings);

	#ifdef USE_ASSIMP
	return AssimpLoadModel(model, path, loadSettings);
	#endif

	LogError("File Type Not Supported: %s", fileType.c_str());
	return false;
}

class MemoryBuffer: public std::streambuf{
public:
    MemoryBuffer(const char* data, size_t size){
        char* p = const_cast<char*>(data);
        setg(p, p, p + size);
    }
};

class MemoryInputStream: public std::istream{
public:
    MemoryInputStream(const char* data, size_t size)
        :std::istream(&buffer), buffer(data, size){}
private:
    MemoryBuffer buffer;
};

bool Model::CreateFromPackage(Model& model, std::string const &path, Package& package, ModelLoadSettings loadSettings){
	model.Clear();

	auto getExtension = [](const std::string& path) -> std::string {
        size_t dotPos = path.rfind('.');
        return (dotPos != std::string::npos) ? path.substr(dotPos + 1) : "";
    };

	std::string fileType = getExtension(path);

	void* data;
	size_t dataSize;
	if(package.ReadFileData(path.c_str(), data, dataSize) == false){
		package.FreeFileData(data);
		return false;
	}

	if(fileType == "modelbin"){
		Assert(false);
		MemoryInputStream mem((char*)data, dataSize);
		cereal::BinaryInputArchive ar(mem);
		model.LoadFrom(ar);

		model.SetPath(path);
		return true;
	}

	#ifdef USE_ASSIMP
	bool result = AssimpLoadModel(model, data, dataSize, fileType.c_str(), loadSettings);
	package.FreeFileData(data);
	return result;
	#endif
}

bool Model::Save(const std::string& outPath, SaveType type){
	if(type == Asset::SaveType::SettingOnly) return false;

    std::ofstream os(outPath, std::ios::binary);
    Assert(os.is_open());

    if(type == Asset::SaveType::AssetBinary){
		Assert(false);
        /*cereal::PortableBinaryOutputArchive ar(os);
        ar(*this);*/
    }
    if(type == Asset::SaveType::FinalBinary){
        cereal::BinaryOutputArchive ar(os);
		SaveTo(ar);
    }

    return true;
}

void Model::CreateMaterialsFromTargets(){
	materials.resize(materialTargets.size());
	for(int i = 0; i < materialTargets.size(); i++){
		Ref<Shader> s = settings.customShader != nullptr ? settings.customShader : AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl");
		Ref<Material> m = CreateRef<Material>(s);

		for(int j = 0; j < materialTargets[i].argNames.size(); j++){
			if(materialTargets[i].texs[j].isFromTexturesArray){
				m->SetTexture(
					materialTargets[i].argNames[j].c_str(),
					textures[materialTargets[i].texs[j].texIndex]
				);
			} else {
				m->SetTexture(
					materialTargets[i].argNames[j].c_str(),
					AssetManager::Get().LoadAsset<Texture2D>(materialTargets[i].texs[j].extPath)
				);
			}
		}

		materials[i] = m;
	}
}

void Model::SaveTo(cereal::BinaryOutputArchive& ar){
    ar(renderTargets);
    ar(meshs);
	ar(textures);
	//ar(materials);
	ar(materialTargets);
    ar(matrixs);
	ar(animationClips);
	ar(skeleton);
	ar(settings);

    /*int texSize = textures.size();
    ar(texSize);

    for(int i = 0; i < texSize; i++){
        int w = textures[i]->Width();
        int h = textures[i]->Height();
        ar(w);
        ar(h);

        std::vector<uint8_t> pixelData;
        textures[i]->GetPixelData(pixelData);

        std::vector<uint8_t> pngData;

        stbi_write_png_to_func(
            [](void* ctx, void* data, int size) {
                auto* out = static_cast<std::vector<uint8_t>*>(ctx);
                uint8_t* bytes = (uint8_t*)data;
                out->insert(out->end(), bytes, bytes + size);
            },
            &pngData,
            w, h, 4,
            pixelData.data(),
            0
        );

        int pngSize = pngData.size();
        ar(pngSize);
        ar(cereal::binary_data(pngData.data(), pngSize));
    }*/
}

void Model::LoadFrom(cereal::BinaryInputArchive& ar){
    ar(renderTargets);
    ar(meshs);
	ar(textures);
    //ar(materials);
	ar(materialTargets);
    ar(matrixs);
	ar(animationClips);
	ar(skeleton);
	ar(settings);

	CreateMaterialsFromTargets();

	/*if(settings.generateColliderData){
		modelShapeData = CreateMeshShapeData(*this);
	}

    int texSize;
    ar(texSize);
    textures.resize(texSize);
    for(int i = 0; i < texSize; i++){
        int w, h;
        ar(w);
        ar(h);

        int pngSize;
        ar(pngSize);

        std::vector<uint8_t> pngData(pngSize);
        ar(cereal::binary_data(pngData.data(), pngSize));

        int ow, oh, nch;
        unsigned char* decoded = stbi_load_from_memory(
            pngData.data(), pngSize,
            &ow, &oh, &nch, 4
        );

        if(!decoded){
			Assert(false); //throw std::runtime_error("Failed to decode PNG in Model::LoadTo");
        }

        textures[i] = Texture2D::CreateFromRaw(decoded, 0, w, h, TextureDataType::UnsignedByte, {});

        stbi_image_free(decoded);
    }*/
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