#include "OD/pch.h"
#include "Prefab.h"
#include "OD/Core/Application.h"
#include "OD/Core/ImGui.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Editor/Editor.h"

namespace OD{

Prefab::Prefab(){

}

void Prefab::OnGui(){
    if(path.empty() == false || path != "Memory"){
		auto* editor = Application::GetModuleByType<Editor>();
		auto* framebuffer = editor->AssetPreviewFramebuffer();
		editor->SetPrefabAssetPreview(path);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		float aspect = framebuffer->Width() / framebuffer->Height();
		ImGui::Image(framebuffer->ColorAttachmentId(0), ImVec2(viewportPanelSize.x, viewportPanelSize.x * aspect), ImVec2(0, 1), ImVec2(1, 0));
	} else {
		ImGui::Text("Can not preview this model!!!");
	}
}

//TODO: Create a base scene and load from this base scene
bool Prefab::LoadFromFile(const std::string& inpath){
    path = inpath;
    if(scene != nullptr) delete scene;

    scene = new Scene(true);
    root = scene->InstantiatePrefab(path.c_str());

    return true;
}

std::vector<std::string> Prefab::GetFileAssociations(){
    return {".prefab"};
}

}