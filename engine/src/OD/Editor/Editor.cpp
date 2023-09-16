#include "Editor.h"
#include "OD/Core/ImGui.h"
#include "OD/Utils/PlatformUtils.h"
#include "OD/RendererSystem/CameraComponent.h"
#include "OD/Core/Input.h"
#include <ImGuizmo/ImGuizmo.h>
#include <glm/gtx/matrix_decompose.hpp>

namespace OD{

void Editor::OnInit(){

}

void Editor::OnUpdate(float deltaTime){

}

void Editor::OnRender(float deltaTime){

}

void CreateDockableWindow(const char* title, bool* open){
    if(ImGui::Begin(title, open)){
        ImGui::Text("This is a dockable window.");
    }
    ImGui::End();
}

void DrawMainPanel(){
    static bool open = true;
    if (open){
        ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        dockspace_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        dockspace_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::SetNextWindowBgAlpha(0); // Transparent background
        ImGui::Begin("DockSpace", &open, dockspace_flags);
        ImGui::PopStyleVar(3);

        // Dockspace layout
        if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable){
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        }

        if(ImGui::BeginMainMenuBar()){
            if(ImGui::BeginMenu("File")){
                if(ImGui::MenuItem("Open", "Ctrl+O")) {}
                if(ImGui::MenuItem("Save", "Ctrl+S")) {}
                if(ImGui::MenuItem("Exit", "Alt+F4")) { }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        static bool showDockableWindow1 = true;
        static bool showDockableWindow2 = true;

        if(showDockableWindow1)
            CreateDockableWindow("Dockable Window 1", &showDockableWindow1);

        if(showDockableWindow2)
            CreateDockableWindow("Dockable Window 2", &showDockableWindow2);
    }

    if(open) ImGui::End();
}

void Editor::DrawMainPanel(){
    ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    if(ImGui::BeginMainMenuBar()){
        if(ImGui::BeginMenu("File")){
            if(ImGui::MenuItem("New", "Ctrl+N")){ 
                Scene* scene = SceneManager::Get().NewScene();
                scene->Start();
                _sceneHierarchyPanel.UnselectContext();
            }

            if(ImGui::MenuItem("Open...", "Ctrl+O")){ 
                std::string path = FileDialogs::OpenFile(""); 
                if(path.empty() == false){
                    Scene* scene = SceneManager::Get().NewScene();
                    scene->Load(path.c_str());
                    scene->Start();
                }
                _sceneHierarchyPanel.UnselectContext();
            }

            if(ImGui::MenuItem("Save As", "Ctrl+Shift+S")){
                std::string path = FileDialogs::SaveFile("");
                if(path.empty() == false){
                    Scene* scene = SceneManager::Get().activeScene();
                    scene->Save(path.c_str());
                } 
            }

            if(ImGui::MenuItem("Exit", "Alt+F4")){ 
                Application::Quit(); 
            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    _sceneHierarchyPanel.SetScene(SceneManager::Get().activeScene());
    _sceneHierarchyPanel.OnGui(&_showSceneHierarchy, &_showInspector);

    DrawGizmos();
}

void Editor::DrawGizmos(){
    Entity camE = SceneManager::Get().activeScene()->GetMainCamera2();
    if(_sceneHierarchyPanel.selectionContext().IsValid() && camE.IsValid() && camE.HasComponent<CameraComponent>() && _gizmoType != Editor::GizmosType::None){
        CameraComponent& cam = camE.GetComponent<CameraComponent>();
        TransformComponent& t = camE.GetComponent<TransformComponent>();
        cam.UpdateCameraData(t);

        ImGuizmo::Enable(true);

        ImGuizmo::SetOrthographic(false);
        //ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
        //ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        
        Matrix4 view = cam.camera().view;
        Matrix4 projection = cam.camera().projection;

        TransformComponent& tc = _sceneHierarchyPanel.selectionContext().GetComponent<TransformComponent>();
        Matrix4 trans = tc.globalModelMatrix();

        bool snap = Input::IsKey(KeyCode::Control);
        float snapValue = 1;
        if(_gizmoType == Editor::GizmosType::Rotation) snapValue = 45;

        float snapValues[3] = {snapValue, snapValue, snapValue};

        ImGuizmo::OPERATION gizmoType = ImGuizmo::OPERATION::TRANSLATE;
        if(_gizmoType == Editor::GizmosType::Rotation) gizmoType = ImGuizmo::OPERATION::ROTATE;
        if(_gizmoType == Editor::GizmosType::Scale) gizmoType = ImGuizmo::OPERATION::SCALE;

        ImGuizmo::Manipulate(
            view.raw(),
            projection.raw(),
            gizmoType, 
            ImGuizmo::LOCAL,
            trans.raw(),
            nullptr,
            (snap ? snapValues : nullptr)
        );

        if(ImGuizmo::IsUsing()){
            glm::vec3 s;
            glm::quat r;
            glm::vec3 t;
            glm::vec3 sk;
            glm::vec4 p;
            glm::decompose((glm::mat4)trans, s, r, t, sk, p);

            if(_gizmoType == Editor::GizmosType::Translation) tc.position(t);
            if(_gizmoType == Editor::GizmosType::Rotation) tc.rotation(r);
            if(_gizmoType == Editor::GizmosType::Scale) tc.localScale(s);
        }
    }

    if(Input::IsKey(KeyCode::Q)) _gizmoType = Editor::GizmosType::None;
    if(Input::IsKey(KeyCode::W)) _gizmoType = Editor::GizmosType::Translation;
    if(Input::IsKey(KeyCode::E)) _gizmoType = Editor::GizmosType::Rotation;
    if(Input::IsKey(KeyCode::R)) _gizmoType = Editor::GizmosType::Scale;
}

void Editor::OnGUI(){
    DrawMainPanel();
}

void Editor::OnResize(int width, int height){

}

}