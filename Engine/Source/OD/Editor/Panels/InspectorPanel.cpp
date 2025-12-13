#include "OD/pch.h"
#include "InspectorPanel.h"
#include "OD/Editor/Editor.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Undo.h"
#include "OD/Scene/Scripts.h"
#include "OD/Scene/SceneUndos.h"
#include "OD/RenderPipeline/CameraComponent.h"
#include "OD/RenderPipeline/LightComponent.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/EnvironmentComponent.h"
#include "OD/Physics/PhysicsSystem.h"

namespace OD{

InspectorPanel::InspectorPanel(){
    name = "InspectorPanel";
    show = true;
}

void InspectorPanel::OnGui(){
    if(scene == nullptr) return;

    //if(ImGui::Begin("Inspector")){
    ImGui::Begin("Inspector");
    if(scene->IsValid(editor->selectionEntity) && editor->selectionOnAsset == false){
        InfoComponent& info = scene->GetComponent<InfoComponent>(editor->selectionEntity);
        if(info.Type() == EntityType::PrefabRoot && ImGui::Button("GoTo")){
            editor->contentBrowserPanel.GoTo(info.PrefabPath());
            ImGui::Separator();
        }

        DrawComponents(editor->selectionEntity);
        ImGui::Separator();
        ImGui::Spacing();
        ShowAddComponent(editor->selectionEntity);
    } else if(editor->selectionOnAsset == true && editor->selectionAsset != nullptr){
        if(ImGui::Button("GoTo")){
            editor->contentBrowserPanel.GoTo(editor->selectionAsset->Path());
        }
        ImGui::Separator();
        editor->selectionAsset->OnGui();
    }
    ImGui::End();
}

/*template<typename T>
void DrawComponent(Entity e, const char* name){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(scene->HasComponent<T>(e)){
        bool removeComponent = false;

        //ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name);
        //ImGui::SameLine(ImGui::GetWindowWidth() - 25.0f);
        //if(ImGui::Button("+", ImVec2(20, 20))){
        //    ImGui::OpenPopup("ComponentSettings");
        //}
        //ImGui::PopStyleVar();
        //if(ImGui::BeginPopup("ComponentSettings")){
        //    if(ImGui::MenuItem("Remove Component")){
        //        removeComponent = true;
        //    }
        //    ImGui::EndPopup();
        //}

        if(ImGui::BeginPopupContextItem()){
            if(ImGui::MenuItem("Remove Component")){
                removeComponent = true;
            }
            ImGui::EndPopup();
        }

        if(open){
            T::OnGui(e, *scene);
            ImGui::TreePop();
        }

        if(removeComponent){
            scene->RemoveComponent<T>(e);
        }
    }
}*/

template<typename T, typename UIFunction>
void DrawComponent(Entity e, Scene& scene, const char* name, UIFunction function){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        /*ImGuiTreeNodeFlags_DefaultOpen 
        |*/ ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(scene.HasComponent<T>(e)){
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name);

        if(open){
            function(e, scene);
            ImGui::TreePop();
        }
    }
}

void InspectorPanel::ComponentOptionsMenu(Entity e, SceneManager::SerializeFuncs &f, bool& removeComponent){
    if(ImGui::BeginPopupContextItem()){
        if(ImGui::MenuItem("Remove Component")){
            removeComponent = true;
        }
        if(ImGui::MenuItem("Copy Component")){
            copyComponentData.target = e;
            copyComponentData.funcs = f;
        }
        if(ImGui::MenuItem("Paste Component") && scene->IsValid(copyComponentData.target)){
            copyComponentData.funcs.copyComponent(copyComponentData.target, e, *scene, *scene);
        }
        ImGui::EndPopup();
    }
}

void InspectorPanel::DrawComponentFromCoreComponents(Entity e, std::string name, SceneManager::SerializeFuncs &f){
    std::hash<std::string> hasher;

    const ImGuiTreeNodeFlags treeNodeFlags = 
        //ImGuiTreeNodeFlags_DefaultOpen | 
        ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(f.hasComponent(e, *scene)){
        bool removeComponent = false;

        bool open = ImGui::TreeNodeEx((void*)hasher(name), treeNodeFlags, name.c_str());

        ComponentOptionsMenu(e, f, removeComponent);

        if(open){
            auto entityType = scene->GetComponent<InfoComponent>(e).Type();
            if(entityType != EntityType::Stand) ImGui::BeginDisabled(true);
            f.onGui(e, *scene);
            if(entityType != EntityType::Stand) ImGui::EndDisabled();

            ImGui::TreePop();
        }

        if(removeComponent){
            //e.RemoveComponent<T>();
            f.removeComponent(e, *scene);
        }
    }
}

void InspectorPanel::DrawComponentFromSerializeFuncs(Entity e, std::string name, SceneManager::SerializeFuncs &sf){
    const ImGuiTreeNodeFlags treeNodeFlags = 
        //ImGuiTreeNodeFlags_DefaultOpen | 
        ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    if(sf.hasComponent(e, *scene)){
        bool removeComponent = false;
        std::hash<std::string> hasher;

        bool open = ImGui::TreeNodeEx((void*)hasher(name), treeNodeFlags, name.c_str());

        ComponentOptionsMenu(e, sf, removeComponent);

        if(open){
            auto entityType = scene->GetComponent<InfoComponent>(e).Type();
            if(entityType != EntityType::Stand) ImGui::BeginDisabled(true);
            sf.onGui(e, *scene);
            if(entityType != EntityType::Stand) ImGui::EndDisabled();

            ImGui::TreePop();
        }

        if(removeComponent){
            //e.RemoveComponent<T>();
            sf.removeComponent(e, *scene);
        }
    }
}

#ifdef _WIN32
#define _strcpy(a, b, c) strcpy_s(a, b, c)
#else 
#define _strcpy(a, b, c) strcpy(a, c)
#endif

void InspectorPanel::DrawComponents(Entity entity){
    TransformComponent& transform = scene->GetComponent<TransformComponent>(entity);
    InfoComponent& info = scene->GetComponent<InfoComponent>(entity);
    EntityType entityType = scene->GetComponent<InfoComponent>(entity).Type();

    DrawComponent<InfoComponent>(entity, *scene, "Info", [&](Entity e, Scene& scene){
        if(entityType != EntityType::Stand) ImGui::BeginDisabled(true);
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));

        auto old = info;

        bool selfEnable = scene.HasComponent<SelfDisable>(e) == false;

        if(ImGui::Checkbox("Enable", &selfEnable)){
            if(selfEnable){
                scene.RemoveComponentRecursive<SelfDisable>(e);
            } else {
                scene.AddComponentRecursive<SelfDisable>(e);
            }
        }
        
        //ImGui::SameLine();
        
        _strcpy(buffer, sizeof(buffer), info.name.c_str());
        if(ImGui::InputText("Name", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)){
            info.name = std::string(buffer);
            UndoManager::Get().Execute(CreateScope<UndoValueComponentCommand<InfoComponent>>(&scene, e, old, info));
        }

        /*bool textEdited = false;
        if (ImGui::InputText("Name", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            textEdited = true;  // User pressed Enter after editing
        } else if (ImGui::IsItemDeactivatedAfterEdit()) {
            textEdited = true;  // User finished editing by losing focus (clicked away)
        }
        if (textEdited) {
            info.name = std::string(buffer);
            UndoManager::Get().Execute(CreateScope<UndoValueComponentCommand<InfoComponent>>(&scene, e, old, info));
        }*/

        _strcpy(buffer, sizeof(buffer), info.tag.c_str());
        if(ImGui::InputText("Tag", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)){
            info.tag = std::string(buffer);
            UndoManager::Get().Execute(CreateScope<UndoValueComponentCommand<InfoComponent>>(&scene, e, old, info));
        }

        ImGui::Text("Id: %zd", (size_t)e);
        //ImGui::Text("Type: %d", info.Type());

        //TODO: Update this
        /*auto& layersName = GetGlobalSceneData().layerNames;
        auto curSelected = layersName[(int)info.layer];
        if(ImGui::BeginCombo("Layer", curSelected.c_str())){
            for(int i = 0; i < layersName.size(); i++){
                bool isSelected = curSelected == layersName[i];
                if(ImGui::Selectable(layersName[i].c_str(), isSelected)){
                    curSelected = layersName[i];
                    info.layer = (Layers)i;
                }
                if(isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }*/
        ImGui::DrawLayer("Layer", info.layer, GetGlobalSceneData().layerNames, true);
        //ImGui::DrawEnumCombo<Layers>("Layer", info.layer, layersName, layersName.size());

        if(entityType != EntityType::Stand) ImGui::EndDisabled();
    });

    DrawComponent<TransformComponent>(entity, *scene, "Transform", [&](Entity e, Scene& scene){
        bool beginDisable = entityType == EntityType::PrefabChild;
        if(entityType == EntityType::PrefabRoot && transform.HasParent() && scene.GetComponent<InfoComponent>(transform.Parent()).Type() == EntityType::PrefabChild)
            beginDisable = true;

        if(beginDisable) ImGui::BeginDisabled(true);

        #ifdef ExperimentalTransformOptimzation
        if(ImGui::Checkbox("isCollection", &transform.isCollection)){
            transform.UpdateGlobalTransformCacheIfNeeded();
        }
        if(transform.isCollection) return;
        #endif

        auto old = transform;
        
        /*if(scene.HasComponent<RigidbodyComponent>(e)){
            RigidbodyComponent& rb = scene.GetComponent<RigidbodyComponent>(e);
            float p[] = {rb.Position().x, rb.Position().y, rb.Position().z};
            if(ImGui::DragFloat3("Position", p, 0.5f)){
                rb.Position(Vector3(p[0], p[1], p[2]));
            }
        } else {*/
            /*
            float p[] = {transform.LocalPosition().x, transform.LocalPosition().y, transform.LocalPosition().z};
            if(ImGui::DragFloat3("Position", p, 0.5f, 0, 0, "%.4f")){
                transform.LocalPosition(Vector3(p[0], p[1], p[2]));
                UndoManager::Get().Execute(CreateScope<UndoValueComponentCommand<TransformComponent>>(&scene, e, old, transform));
            }
        //}  

        float r[] = {transform.LocalEulerAngles().x, transform.LocalEulerAngles().y, transform.LocalEulerAngles().z};
        if(ImGui::DragFloat3("Rotation", r, 0.5f, 0, 0, "%.4f")){
            transform.LocalEulerAngles(Vector3(r[0], r[1], r[2]));
            UndoManager::Get().Execute(CreateScope<UndoValueComponentCommand<TransformComponent>>(&scene, e, old, transform));
        }  

        float s[] = {transform.LocalScale().x, transform.LocalScale().y, transform.LocalScale().z};
        if(ImGui::DragFloat3("Scale", s, 0.5f, 0, 0, "%.4f")){
            transform.LocalScale(Vector3(s[0], s[1], s[2]));
            UndoManager::Get().Execute(CreateScope<UndoValueComponentCommand<TransformComponent>>(&scene, e, old, transform));
        }
        */

        // Calculate label width to mimic default ImGui::DragFloat3 spacing
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float fontSize = ImGui::GetFontSize();
        // Use the longest label width plus padding to match default DragFloat3 label spacing
        float longestLabelWidth = math::max(math::min(ImGui::CalcTextSize("Position").x, ImGui::CalcTextSize("Rotation").x), ImGui::CalcTextSize("Scale").x);
        float labelWidth = longestLabelWidth + ImGui::GetStyle().ItemInnerSpacing.x + 4.0f; // Mimic default spacing
        float minValueWidth = availableWidth * 0.50f; // Ensure value column is at least 50% of available width
        labelWidth = math::clamp(availableWidth * 0.25f, labelWidth*0.5f, labelWidth*2);

        // Begin table
        if (ImGui::BeginTable("TransformTable", 2, 
                ImGuiTableFlags_SizingStretchSame | 
                ImGuiTableFlags_NoPadOuterX | 
                ImGuiTableFlags_BordersInnerV)
            ) {
            
            // Setup columns
            ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            // Helper lambda to draw a row
            auto DrawTransformRow = [&](const char* label, Vector3& value, const char* id, float dragSpeed, float min = 0.0f, float max = 0.0f) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f); // Slight indent for Unity-like look
                ImGui::TextUnformatted(label);
                ImGui::TableSetColumnIndex(1);
                // Stretch inputs to fill available space, respecting minimum width
                ImGui::PushItemWidth(math::max(minValueWidth, math::min(availableWidth - labelWidth - 10.0f, availableWidth - labelWidth)));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f)); // Tight padding
                
                float v[3] = {value.x, value.y, value.z};
                if (ImGui::DragFloat3(id, v, dragSpeed, min, max, "%.3f")) {
                    value = Vector3(v[0], v[1], v[2]);
                    UndoManager::Get().Execute(CreateScope<UndoValueComponentCommand<TransformComponent>>(&scene, e, transform, transform));
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Adjust %s (X, Y, Z)", label);
                }
                ImGui::PopStyleVar();
                ImGui::PopItemWidth();
            };

            // Position
            Vector3 position = transform.LocalPosition();
            DrawTransformRow("Position", position, "##Position", 0.5f);
            transform.LocalPosition(position);

            // Rotation
            Vector3 rotation = transform.LocalEulerAngles();
            DrawTransformRow("Rotation", rotation, "##Rotation", 0.5f);
            transform.LocalEulerAngles(rotation);

            // Scale
            Vector3 scale = transform.LocalScale();
            DrawTransformRow("Scale", scale, "##Scale", 0.5f, 0.0f, 0.0f);
            transform.LocalScale(scale);

            ImGui::EndTable();
        }
    
        if(beginDisable) ImGui::EndDisabled();
    });

    ImGui::Spacing();
    //ImGui::Separator();
    ImGui::Spacing();

    //if(entityType != EntityType::Stand) ImGui::BeginDisabled(true);
    
    for(auto& i: SceneManager::Get().coreComponentsSerializer){
        //LogInfo("%s", i.first.c_str());
        //DrawComponentFromCoreComponents(entity, i.first, i.second);

        std::string displayName = i.second.displayName; //i.first.substr(i.first.find_last_of('/') + 1);
        DrawComponentFromCoreComponents(entity, displayName, i.second);
    }

    ImGui::Spacing();
    //ImGui::Separator();
    ImGui::Spacing(); 

    for(auto& i: SceneManager::Get().componentsSerializer){
        //DrawComponentFromSerializeFuncs(entity, i.first, i.second);
        
        std::string displayName = i.second.displayName; //i.first.substr(i.first.find_last_of('/') + 1);
        DrawComponentFromSerializeFuncs(entity, displayName, i.second);
    }

    //if(entityType != EntityType::Stand) ImGui::EndDisabled();
}

void InspectorPanel::ShowAddComponent(Entity entity){
    /*if(ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponent");

    // Set popup style (optional: rounded corners, max size)
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 100), ImVec2(400, 300)); // Min and max size for the popup

     // Static variables for search
    static char searchBuffer[128] = "";
    static std::string searchQuery;

    // Clear search buffer when popup closes
    if (!ImGui::IsPopupOpen("AddComponent")) {
        searchBuffer[0] = '\0'; // Clear the buffer
        searchQuery.clear();    // Clear the query
    }

    if (ImGui::BeginPopup("AddComponent")) {
        // Static buffer for search input
        //static char searchBuffer[128] = "";
        //static std::string searchQuery;

        // Search bar
        ImGui::Text("Search:");
        ImGui::SameLine();
        if (ImGui::InputText("##SearchComponents", searchBuffer, sizeof(searchBuffer))) {
            searchQuery = std::string(searchBuffer);
            std::transform(searchQuery.begin(), searchQuery.end(), searchQuery.begin(), ::tolower);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Create a scrollable child window with fixed height
        ImGui::BeginChild("ComponentList", ImVec2(0, 200), true); // 200 is the fixed height, adjust as needed

        // Filter and display core components
        for (auto& i : SceneManager::Get().coreComponentsSerializer) {
            std::string componentName = i.first;
            std::string componentNameLower = componentName;
            std::transform(componentNameLower.begin(), componentNameLower.end(), componentNameLower.begin(), ::tolower);

            if (searchQuery.empty() || componentNameLower.find(searchQuery) != std::string::npos) {
                if (ImGui::MenuItem(i.first.c_str())) {
                    i.second.addComponent(editor->selectionEntity, *scene);
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Filter and display other components
        for (auto& i : SceneManager::Get().componentsSerializer) {
            std::string componentName = i.first;
            std::string componentNameLower = componentName;
            std::transform(componentNameLower.begin(), componentNameLower.end(), componentNameLower.begin(), ::tolower);

            if (searchQuery.empty() || componentNameLower.find(searchQuery) != std::string::npos) {
                if (ImGui::MenuItem(i.first.c_str())) {
                    i.second.addComponent(editor->selectionEntity, *scene);
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        ImGui::EndChild(); // End scrollable region

        ImGui::EndPopup();
    }*/

    using ComponentEntry = SceneManager::SerializeFuncs;

    if(ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponent");

    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 100), ImVec2(400, 300));

    static char searchBuffer[128] = "";
    static std::string searchQuery;

    if(!ImGui::IsPopupOpen("AddComponent")){
        searchBuffer[0] = '\0';
        searchQuery.clear();
    }

    if(ImGui::BeginPopup("AddComponent")){
        ImGui::Text("Search:");
        ImGui::SameLine();
        if(ImGui::InputText("##SearchComponents", searchBuffer, sizeof(searchBuffer))){
            searchQuery = std::string(searchBuffer);
            std::transform(searchQuery.begin(), searchQuery.end(), searchQuery.begin(), ::tolower);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild("ComponentList", ImVec2(0, 200), true);

        auto extractFilteredComponents = [&](const auto& componentMap){
            std::vector<std::pair<std::vector<std::string>, ComponentEntry>> filtered;
            for(auto& [name, entry] : componentMap){
                std::string _name = entry.displayName;
                std::string fullName = entry.groupName.empty() ? _name: entry.groupName + "/" + _name;
                std::string fullNameLower = fullName;
                std::transform(fullNameLower.begin(), fullNameLower.end(), fullNameLower.begin(), ::tolower);

                if(searchQuery.empty() || fullNameLower.find(searchQuery) != std::string::npos){
                    std::vector<std::string> path;
                    std::stringstream ss(fullName);
                    std::string segment;
                    while(std::getline(ss, segment, '/'))
                        path.push_back(segment);
                    filtered.emplace_back(path, entry);
                }
            }
            return filtered;
        };

        auto filteredCore = extractFilteredComponents(SceneManager::Get().coreComponentsSerializer);
        auto filteredOther = extractFilteredComponents(SceneManager::Get().componentsSerializer);

        // FLAT: when searching, show all entries flattened, first core, then other
        if(!searchQuery.empty()){
            for (auto& [path, entry] : filteredCore) {
                std::string fullName = std::accumulate(std::next(path.begin()), path.end(), path[0],
                    [](const std::string& a, const std::string& b) { return a + "/" + b; });

                if (ImGui::MenuItem(fullName.c_str())) {
                    entry.addComponent(editor->selectionEntity, *scene);
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::Separator();

            for (auto& [path, entry] : filteredOther) {
                std::string fullName = std::accumulate(std::next(path.begin()), path.end(), path[0],
                    [](const std::string& a, const std::string& b) { return a + "/" + b; });

                if (ImGui::MenuItem(fullName.c_str())) {
                    entry.addComponent(editor->selectionEntity, *scene);
                    ImGui::CloseCurrentPopup();
                }
            }
        } 
        // GROUPED: show core first, then other
        else {
            std::function<void(const std::vector<std::pair<std::vector<std::string>, ComponentEntry>>&, size_t)> renderGroup;
            renderGroup = [&](const std::vector<std::pair<std::vector<std::string>, ComponentEntry>>& list, size_t depth) {
                std::map<std::string, std::vector<std::pair<std::vector<std::string>, ComponentEntry>>> groups;
                std::vector<std::pair<std::vector<std::string>, ComponentEntry>> leafItems;

                for (const auto& [path, entry] : list) {
                    if (depth + 1 < path.size()) {
                        groups[path[depth]].push_back({ path, entry });
                    } else {
                        leafItems.push_back({ path, entry });
                    }
                }

                for (auto& [groupName, groupList] : groups) {
                    if (ImGui::BeginMenu(groupName.c_str())) {
                        renderGroup(groupList, depth + 1);
                        ImGui::EndMenu();
                    }
                }

                for (auto& [path, entry] : leafItems) {
                    if (ImGui::MenuItem(path.back().c_str())) {
                        entry.addComponent(editor->selectionEntity, *scene);
                        ImGui::CloseCurrentPopup();
                    }
                }
            };

            // Render core first
            if (!filteredCore.empty()) {
                renderGroup(filteredCore, 0);
                if (!filteredOther.empty())
                    ImGui::Separator(); // Visual separator
            }

            // Render non-core after
            if (!filteredOther.empty()) {
                renderGroup(filteredOther, 0);
            }
        }

        ImGui::EndChild();
        ImGui::EndPopup();
    }

}

}