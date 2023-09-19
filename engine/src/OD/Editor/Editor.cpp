#include "Editor.h"
#include "OD/Core/ImGui.h"
#include "OD/Utils/PlatformUtils.h"
#include "OD/RendererSystem/CameraComponent.h"
#include "OD/Core/Input.h"
#include <imgui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>
#include <glm/gtx/matrix_decompose.hpp>
#include "OD/Core/Application.h"

namespace OD{

Editor* Editor::instance;

void Editor::OnInit(){
    FrameBufferSpecification framebufferSpecification = {Application::screenWidth(), Application::screenHeight()};
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGB}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8, true};
    _framebuffer = new Framebuffer(framebufferSpecification);
    _framebuffer->Invalidate();

    _viewportSize.x = _framebuffer->width();
    _viewportSize.y = _framebuffer->height();

    instance = this;

    _cam.OnStart();
    _cam.moveSpeed = 60;
}

void Editor::OnUpdate(float deltaTime){
    if(SceneManager::Get().activeScene()->running()){
        SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->overrideCamera(nullptr);
    } else {
        _cam.OnUpdate();
        _cam.cam.SetPerspective(45, 0.1f, 10000.0f, _viewportSize.x, _viewportSize.y);
        _cam.cam.view = _cam.transform.GetLocalModelMatrix().inverse();
        SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->overrideCamera(&_cam.cam);
    }

    SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->SetOutFrameBuffer(_framebuffer);
    if(_framebuffer->IsValid() && _viewportSize.x != 0 && _viewportSize.y != 0) _framebuffer->Resize(_viewportSize.x, _viewportSize.y);

    HandleShotcuts();
}

void Editor::OnRender(float deltaTime){

}

void Editor::PlayScene(){
    Scene* scene = SceneManager::Get().activeScene();
    if(_curScenePath.empty()) scene->Save("res/temp.scene");
    scene->Start();
}

void Editor::StopScene(){
    Scene* scene = SceneManager::Get().NewScene();
    scene->Load(_curScenePath.empty() ? "res/temp.scene" : _curScenePath.c_str());
    _sceneHierarchyPanel.UnselectContext();
}

void Editor::NewScene(){
    if(SceneManager::Get().activeScene()->running()) return;

    Scene* scene = SceneManager::Get().NewScene();
    _sceneHierarchyPanel.UnselectContext();
    _curScenePath = "";
}   

void Editor::OpenScene(){
    if(SceneManager::Get().activeScene()->running()) return;

    std::string path = FileDialogs::OpenFile(""); 
    if(path.empty() == false){
        Scene* scene = SceneManager::Get().NewScene();
        scene->Load(path.c_str());
        _curScenePath = scene->path();
    }
    _sceneHierarchyPanel.UnselectContext();
}

void Editor::SaveAsScene(){
    if(SceneManager::Get().activeScene()->running()) return;

    std::string path = FileDialogs::SaveFile("");
    if(path.empty() == false){
        Scene* scene = SceneManager::Get().activeScene();
        scene->Save(path.c_str());
        _curScenePath = path;
    } 
}

void Editor::HandleShotcuts(){
    if(Input::IsKey(KeyCode::F1)) PlayScene();
    if(Input::IsKey(KeyCode::F2)) StopScene();

    if(SceneManager::Get().activeScene()->running()) return;

    if(Input::IsKey(KeyCode::Q)) _gizmoType = Editor::GizmosType::None;
    if(Input::IsKey(KeyCode::W)) _gizmoType = Editor::GizmosType::Translation;
    if(Input::IsKey(KeyCode::E)) _gizmoType = Editor::GizmosType::Rotation;
    if(Input::IsKey(KeyCode::R)) _gizmoType = Editor::GizmosType::Scale;
}

void Editor::DrawMainWorkspace(){
    _sceneHierarchyPanel.SetScene(SceneManager::Get().activeScene());
    _sceneHierarchyPanel.OnGui(&_showSceneHierarchy, &_showInspector);

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Renderer Stats");
    ImGui::Text("DrawCalls: %d", Renderer::drawCalls);
    ImGui::Text("Vertices: %dk", Renderer::vertices / 1000);
    ImGui::Text("Tris: %dk", Renderer::tris / 1000);
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::Begin("Viewport");

    auto& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    _viewportSize.x = viewportPanelSize.x;
    _viewportSize.y = viewportPanelSize.y;

    uint32_t textureId = _framebuffer->ColorAttachmentId(0);
    ImGui::Image((void*)(uint64_t)textureId, ImVec2(viewportPanelSize.x, viewportPanelSize.y), ImVec2(0, 1), ImVec2(1, 0));

    //_framebuffer->Resize((int)viewportPanelSize.x, (int)viewportPanelSize.y);

    DrawGizmos();

    ImGui::End();
    ImGui::PopStyleVar();
    //io.ConfigWindowsMoveFromTitleBarOnly = false;
}

void Editor::DrawMainMenuBar(){
    if(ImGui::BeginMainMenuBar()){
        if(ImGui::BeginMenu("File")){
            if(ImGui::MenuItem("New", "Ctrl+N")) NewScene();
            if(ImGui::MenuItem("Open...", "Ctrl+O")) OpenScene();
            if(ImGui::MenuItem("Save As", "Ctrl+Shift+S")) SaveAsScene();
            if(ImGui::MenuItem("Exit", "Alt+F4")) Application::Quit(); 

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Runtime")){
            if(ImGui::MenuItem("Play", "F1")) PlayScene();
            if(ImGui::MenuItem("Stop", "F2")) StopScene();
            
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Editor::DrawMainPanel(){
    //DrawGizmos();

    ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    /*static bool open = true;
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
    }*/

    DrawMainMenuBar();

    bool workspace;
    bool code;

    ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration;
    float height = ImGui::GetFrameHeight();
    if(ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flags)){
        if(ImGui::BeginMenuBar()){
            ImGui::Text("Happy secondary menu bar");
            ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
            //ImGui::Button("Test", ImVec2(0, height-1));
            
            ImGui::BeginTabBar("BeginTabBar");
            
            if(ImGui::BeginTabItem("Workspace", &workspace)){
                ImGui::EndTabItem();
                DrawMainWorkspace();
            }

            if(ImGui::BeginTabItem("Workspace2", &code)){
                ImGui::Begin("Code");
                ImGui::End();

                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
            
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();

    if(ImGui::BeginViewportSideBar("##MainStatusBar", NULL, ImGuiDir_Down, height, window_flags)){
        if(ImGui::BeginMenuBar()){
            ImGui::Text("Happy status bar");
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();

    
    //_sceneHierarchyPanel.SetScene(SceneManager::Get().activeScene());
    //_sceneHierarchyPanel.OnGui(&_showSceneHierarchy, &_showInspector);

    /*
    if(workspace){
        DrawMainWorkspace();
    }
    if(code){
        ImGui::Begin("Code");
        ImGui::End();
    }*/

    //DrawMainWorkspace();

    //DrawGizmos();

    //}
    //if(open) ImGui::End();
}

void Editor::DrawGizmos(){
    if(_sceneHierarchyPanel.selectionContext().IsValid() == false) return;
    if(_gizmoType == Editor::GizmosType::None) return;

    Camera cam = _cam.cam;

    if(SceneManager::Get().activeScene()->running()){
        Entity camE = SceneManager::Get().activeScene()->GetMainCamera2();
        if(camE.IsValid() == false) return;

        CameraComponent& cameraComponent = camE.GetComponent<CameraComponent>();
        cam = cameraComponent.camera();
    }
    
    ImGuizmo::Enable(true);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
    //ImGuiIO& io = ImGui::GetIO();
    //ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    Matrix4 view = cam.view;
    Matrix4 projection = cam.projection;

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

void Editor::OnGUI(){
    DrawMainPanel();
}

void Editor::OnResize(int width, int height){

}

}