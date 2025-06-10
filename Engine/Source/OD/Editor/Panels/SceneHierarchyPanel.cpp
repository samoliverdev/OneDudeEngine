#include "SceneHierarchyPanel.h"
#include "OD/Editor/Editor.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Input.h"
#include "OD/Scene/Scripts.h"
#include "OD/RenderPipeline/CameraComponent.h"
#include "OD/RenderPipeline/LightComponent.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/RenderPipeline/EnvironmentComponent.h"
#include "OD/Physics/PhysicsSystem.h"
#include "OD/Platform/Platform.h"
#include "OD/Graphics/Model.h"
//#include "OD/AnimationSystem/Animator.h"
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include <string>
#include <imgui/imgui_internal.h>

namespace OD{

bool _toDestroy = false;
entt::entity _toDestroyEntity;

entt::entity _savePrefab = entt::null;

SceneHierarchyPanel::SceneHierarchyPanel(){
    name = "SceneHierarchyPanel";
    show = true;
}

void SceneHierarchyPanel::OnGui(){
    if(scene == nullptr) return;

    //if(ImGui::Begin("Scene Hierarchy")){
    ImGui::Begin("Scene Hierarchy");
        /*_scene->GetRegistry().sort<InfoComponent>([](const auto &lhs, const auto &rhs) {
            return lhs.id() < rhs.id();
        });*/

        scene->GetRegistry().sort<InfoComponent>([](const entt::entity lhs, const entt::entity rhs) {
            return lhs < rhs;
        });

        /*Vector3 camPos = scene->GetMainCamera2().GetComponent<TransformComponent>().Position();
        scene->GetRegistry().sort<TransformComponent>([&](const TransformComponent& lhs, const TransformComponent& rhs) {
            return math::distance(camPos, lhs.localPosition) < math::distance(camPos, rhs.localPosition);
        });*/

        auto view = scene->GetRegistry().view<TransformComponent, InfoComponent>();
        view.use<InfoComponent>();
        for(auto e: view){
            //Entity _e(e, scene);
            DrawEntityNode(e, true);
        }

        /*auto v = _scene->GetRegistry().view<entt::entity>();
        for(auto it = v.rbegin(), last = v.rend(); it != last; ++it){
            Entity _e(*it, _scene);
            DrawEntityNode(_e, true);
        }*/
        
        /*auto v = _scene->GetRegistry().view<entt::entity>();
        std::for_each(v.rbegin(), v.rend(), [&](entt::entity e){
            Entity _e(e, _scene);
            DrawEntityNode(_e, true);
        });*/

        /*_scene->GetRegistry().view<entt::entity>().each([&](auto e){
            Entity _e(e, _scene);
            DrawEntityNode(_e, true);
        });*/

        /*if(toDestroy.IsValid()){
            _scene->DestroyEntity(toDestroy.id());
            _editor->SetSelectionEntity(Entity());
            toDestroy = Entity();
            return;
        }*/
        
        if(_toDestroy){
            LogInfo("To Destroy Entity2: %d", _toDestroyEntity);
            editor->SetSelectionEntity(EntityNull);
            scene->DestroyEntityImmediate(_toDestroyEntity); //scene->DestroyEntity(_toDestroyEntity);
            _toDestroy = false;
            return;
        }

        //if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::GetIO().KeyShift){
        if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !Input::IsKey(KeyCode::LShift)){
            //_editor->_selectionEntity = Entity();
            editor->SetSelectionEntity(EntityNull);
        }

        if(ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)){
            if(ImGui::MenuItem("Create Empty Entity")){
                scene->AddEntity("Empty Entity");
            }
            if(ImGui::MenuItem("Add Prefab")){
                std::string path = Platform::OpenFile("*.prefab");
                if(path.empty() == false) scene->InstantiatePrefab(path.c_str());
            }
            ImGui::EndPopup();
        }

        /*ImGui::Button("test", ImGui::GetContentRegionAvail());
        if(ImGui::BeginDragDropTarget()){
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityMoveDragDrop");
            if(payload != nullptr){
                Entity* path = (Entity*)payload->Data;
                LogInfo("this: %s", path->GetComponent<InfoComponent>().name.c_str());
                if(path->IsValid()){}
            }
            ImGui::EndDragDropTarget();
        }*/

        if(ImGui::IsDragDropActive()){

            ImGui::BeginChild("BottomBar", ImVec2(0,0), false, 0); // Use avail width/height
            //ImGui::Text("Footer");
            ImGui::EndChild();
            if(ImGui::BeginDragDropTarget()){
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityMoveDragDrop");
                if(payload != nullptr){
                    Entity* targetEntity = (Entity*)payload->Data;
                    LogInfo("this: %s", scene->GetComponent<InfoComponent>(*targetEntity).name.c_str());
                    if(scene->IsValid(*targetEntity) && scene->GetComponent<InfoComponent>(*targetEntity).Type() != EntityType::PrefabChild){
                        scene->CleanParent(*targetEntity);
                    }
                }

                const ImGuiPayload* payload2 = ImGui::AcceptDragDropPayload("FILE_MOVE_PAYLOAD");
                if(payload2 != nullptr){
                    std::filesystem::path* path = (std::filesystem::path*)payload2->Data;

                    auto getExtension = [](const std::filesystem::path& path) -> std::string {
                        return path.has_extension() ? path.extension().string() : "";
                    };
                    auto getFileNameWithoutExtension = [](const std::filesystem::path& path) -> std::string {
                        return path.stem().string();
                    };

                    Model m;
                    if(m.HasFileExtension(getExtension(*path))){
                        auto _path = path->string();
                        std::replace(_path.begin(), _path.end(), '\\', '/');

                        Ref<Model> model = AssetManager::Get().LoadAsset<Model>(_path);
                        Entity mEntity = scene->AddEntity(getFileNameWithoutExtension(*path));
                        ModelRendererComponent& mRenderer = scene->AddComponent<ModelRendererComponent>(mEntity);
                        mRenderer.SetModel(model);
                    }

                    if(getExtension(*path) == ".prefab"){
                        scene->InstantiatePrefab(path->string().c_str());
                        //scene->InstantiatePrefab(*AssetManager::Get().LoadAsset<Prefab>(path->string()));
                    }

                    LogInfo("Reciving File: %s", path->string().c_str());
                }

                ImGui::EndDragDropTarget();
            }

        }
        
        //ImGui::End();
        //ImGui::EndChild();
    //}
    ImGui::End();

    if(Input::IsKeyDown(KeyCode::Delete) && editor->SelectedEntitiesCount() > 0){
        for(auto& i: editor->_selectedEntities){
            scene->DestroyEntity(i);
        }
        editor->UnselectAll();
    }
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity, bool root){
    Assert(scene->IsValid(entity));

    TransformComponent& transform = scene->GetComponent<TransformComponent>(entity);
    InfoComponent& info = scene->GetComponent<InfoComponent>(entity);

    Entity children = EntityNull;

    if(root && transform.HasParent()) return;

    if(showHide == false){
        if(info.hidden) return;    
        if(scene->HasComponent<HideInEditor>(entity)) return;
    }

    //ImGui::Text(info.name.c_str());

    EntityType entityType = info.Type();

    int validChildren = 0;
    for(auto i: transform.Children()){
        //Entity e(i, entity.GetScene());
        if(scene->GetComponent<InfoComponent>(i).hidden == false) validChildren += 1;
    }
    if(entityType != EntityType::Stand) validChildren = 0;

    ImGuiTreeNodeFlags flags = 
        (editor->_selectedEntities.count(entity)? ImGuiTreeNodeFlags_Selected : 0) | 
        (validChildren > 0 ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf);
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

    if(entityType == EntityType::PrefabRoot) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(55, 125, 205)));
    if(entityType == EntityType::PrefabChild) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(55, 155, 205)));

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, info.name.c_str());
    
    if(entityType != EntityType::Stand) ImGui::PopStyleColor();
    
    if(scene->IsValid(entity) && ImGui::BeginDragDropSource()){
        ImGui::SetDragDropPayload("EntityMoveDragDrop", &entity, sizeof(Entity), ImGuiCond_Once);
        ImGui::EndDragDropSource();
    }

    if(ImGui::IsItemClicked()){
        //bool holdMultSelection = ImGui::GetIO().KeyShift; //Input::IsKey(KeyCode::LShift);
        bool holdMultSelection = Input::IsKey(KeyCode::LShift);// && !ImGui::GetIO().WantCaptureKeyboard;

        if(holdMultSelection){
            editor->AddSelectionEntity(entity);
        } else {    
            editor->SetSelectionEntity(entity);
        } 
    }

    if(ImGui::BeginDragDropTarget()){
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityMoveDragDrop");
        if(payload != nullptr){
            Entity* targetEntity = (Entity*)payload->Data;
            LogInfo("this: %s to: %s", scene->GetComponent<InfoComponent>(entity).name.c_str(), scene->GetComponent<InfoComponent>(*targetEntity).name.c_str());
            if(scene->IsValid(entity) && scene->IsValid(*targetEntity) && entity != *targetEntity){
                children = *targetEntity;
            }
        }
        ImGui::EndDragDropTarget();

        const ImGuiPayload* payload2 = ImGui::AcceptDragDropPayload("FILE_MOVE_PAYLOAD");
        if(payload2 != nullptr){
            std::filesystem::path* path = (std::filesystem::path*)payload2->Data;

            auto getExtension = [](const std::filesystem::path& path) -> std::string {
                return path.has_extension() ? path.extension().string() : "";
            };
            auto getFileNameWithoutExtension = [](const std::filesystem::path& path) -> std::string {
                return path.stem().string();
            };

            Model m;
            if(m.HasFileExtension(getExtension(*path))){
                auto _path = path->string();
                std::replace(_path.begin(), _path.end(), '\\', '/');

                Ref<Model> model = AssetManager::Get().LoadAsset<Model>(_path);
                Entity mEntity = scene->AddEntity(getFileNameWithoutExtension(*path));
                ModelRendererComponent& mRenderer = scene->AddComponent<ModelRendererComponent>(mEntity);
                mRenderer.SetModel(model);
                scene->SetParent(entity, mEntity);
            }

            if(getExtension(*path) == ".prefab"){
                auto instance = scene->InstantiatePrefab(path->string().c_str());
                scene->SetParent(entity, instance);
            }

            LogInfo("Reciving File: %s", path->string().c_str());
        }
        ImGui::EndDragDropTarget();
    }

    bool entityDeleted = false;
    if(ImGui::BeginPopupContextItem()){
        if(ImGui::MenuItem("Create Empty Entity")){
            auto emptyEntity = scene->AddEntity("Empty Entity");
            scene->SetParent(entity, emptyEntity);
        }
        if(ImGui::MenuItem("Add Prefab")){
            std::string path = Platform::OpenFile("*.prefab");
            if(path.empty() == false){
                auto emptyEntity = scene->InstantiatePrefab(path.c_str());
                scene->SetParent(entity, emptyEntity);
            }
        }

        if(info.Type() != EntityType::PrefabChild && ImGui::MenuItem("Delete Entity")){
            entityDeleted = true;
            //_toDestroy = true;
            //_toDestroyEntity = entity.id();
        }

        if(info.Type() == EntityType::Stand && ImGui::MenuItem("Select All Childres")){
            editor->UnselectAll();
            for(auto& i: transform.Children()){
                editor->AddSelectionEntity(i);
            }
        }

        if(info.Type() == EntityType::PrefabRoot && ImGui::MenuItem("Unpack Prefab")){
            scene->UnpackPrefab(entity, false);
        }

        if(info.Type() == EntityType::PrefabRoot && ImGui::MenuItem("Unpack Prefab All")){
            scene->UnpackPrefab(entity, true);
        }

        if(ImGui::MenuItem("Duplicate Entity")){
            scene->DuplicateEntity(entity);
        }

        if(scene->GetComponent<InfoComponent>(entity).Type() == EntityType::Stand && ImGui::MenuItem("Save Prefab")){
            std::string path = Platform::SaveFile("*.prefab");
            if(path.empty() == false){
                auto relativePath = std::filesystem::relative(path, std::filesystem::current_path());
                std::string pathString = relativePath.string();
                std::replace(pathString.begin(), pathString.end(), '\\', '/');

                Scene* scene = SceneManager::Get().GetActiveScene();
                scene->Save(pathString.c_str(), entity);
            } 
        }
        ImGui::EndPopup();
    }

    if(opened){
        if(entityType == EntityType::Stand){
            for(auto i: transform.Children()){
                DrawEntityNode(i, false);
            }
        }

        ImGui::TreePop();
    }

    if(entityDeleted){
        LogInfo("To Destroy Entity: %d", entity);

        scene->DestroyEntityImmediate(entity); //scene->DestroyEntity(entity);
        editor->SetSelectionEntity(EntityNull);

        //toDestroy = entity;
    } else if(
        scene->IsValid(children) 
        && scene->GetComponent<InfoComponent>(entity).Type() == EntityType::Stand 
        && scene->GetComponent<InfoComponent>(children).Type() != EntityType::PrefabChild
    ){
        scene->SetParent(entity, children);
    }
}
}