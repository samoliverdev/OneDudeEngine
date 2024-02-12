#include "Editor.h"
#include "OD/Scene/BaseRenderPipeline.h"
#include "OD/Core/ImGui.h"
#include "OD/Utils/PlatformUtils.h"
#include "OD/RenderPipeline/CameraComponent.h"
#include "OD/RenderPipeline/StandRenderPipeline.h"
#include "OD/Core/Input.h"
#include <imgui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>
#include <glm/gtx/matrix_decompose.hpp>
#include "OD/Core/Application.h"
#include "OD/Core/Instrumentor.h"
#include <filesystem>

namespace OD{

Editor* Editor::instance;

void Editor::OnInit(){
    ImGuiLayer::SetCleanAll(true);

    FrameBufferSpecification framebufferSpecification = {Application::ScreenWidth(), Application::ScreenHeight()};
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGB}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebuffer = new Framebuffer(framebufferSpecification);
    framebuffer->Invalidate();

    viewportSize.x = framebuffer->Width();
    viewportSize.y = framebuffer->Height();

    instance = this;

    editorCam.OnStart();
    editorCam.moveSpeed = 60;

    sceneHierarchyPanel.SetEditor(this);
    inspectorPanel.SetEditor(this);
    contentBrowserPanel.SetEditor(this);

    mainWorkspace.SetEditor(this);
    mainWorkspace.AddPanel(&sceneHierarchyPanel);
    mainWorkspace.AddPanel(&inspectorPanel);
    mainWorkspace.AddPanel(&contentBrowserPanel);
    mainWorkspace.AddPanel(&viewportPanel);
    mainWorkspace.AddPanel(&profilePanel);
    mainWorkspace.AddPanel(&rendererStatsPanel);

    std::ifstream is("res/Editor.Save");
    if(is.fail() == false){
        cereal::JSONInputArchive archive{is};
        archive(cereal::make_nvp("Editor", *this));
    }
}

void Editor::OnExit(){
    //LogInfo("Edito::OnExit");

    std::ofstream os("res/Editor.Save");
    cereal::JSONOutputArchive archive{os};
    archive(cereal::make_nvp("Editor", *this));
}

void Editor::OnUpdate(float deltaTime){
    OD_PROFILE_SCOPE("Editor::OnUpdate");

    if(Input::IsKeyDown(KeyCode::F5)) open = !open;

    BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
    Assert(renderPipeline != nullptr);
    //Assert(dynamic_cast<StandRenderPipeline2*>(renderPipeline));

    if(SceneManager::Get().GetActiveScene()->Running()){
        renderPipeline->SetOverrideCamera(nullptr, Transform());
    } else {
        float width = viewportSize.x;
        float height = viewportSize.y;
        if(open == false){
            width = Application::ScreenWidth();
            height = Application::ScreenHeight();
        }

        editorCam.OnUpdate();
        editorCam.cam.SetPerspective(45, 0.1f, 2000.0f, width, height);
        editorCam.cam.viewPos = editorCam.transform.LocalPosition();
        editorCam.cam.view = math::inverse(editorCam.transform.GetLocalModelMatrix());
        editorCam.cam.frustum = CreateFrustumFromCamera(editorCam.transform, width / height, Mathf::Deg2Rad(45), 0.1f, 2000.0f);
        renderPipeline->SetOverrideCamera(&editorCam.cam, editorCam.transform);
    }

    if(open == false){
        renderPipeline->SetOverrideFrameBuffer(nullptr);
        ImGuiLayer::SetCleanAll(false);
    } else{
        ImGuiLayer::SetCleanAll(true);
        renderPipeline->SetOverrideFrameBuffer(framebuffer);
        framebuffer->Resize(viewportSize.x, viewportSize.y);
    }

    /*if(SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->finalColor()->IsValid()){
        auto[mx, my] = ImGui::GetMousePos();
        mx -= m_ViewportBounds[0].x;
        my -= m_ViewportBounds[0].y;
        Vector2 viewportSize = Vector2(m_ViewportBounds[1].x, m_ViewportBounds[1].y) - Vector2(m_ViewportBounds[0].x, m_ViewportBounds[0].y);
        my = viewportSize.y - my;
        int mouseX = (int)mx;
        int mouseY = (int)my;

        SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->finalColor()->Bind();
        LogInfo("ReadPixel(1): %d",SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->finalColor()->ReadPixel(1, mouseX, mouseY));
        SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->finalColor()->Unbind();
    }*/

    HandleShotcuts();
}

void Editor::OnRender(float deltaTime){

}

void Editor::OnGUI(){
    OD_PROFILE_SCOPE("Editor::OnGUI");

    if(open == false) return;

    DrawMainPanel();
}

void Editor::OnResize(int width, int height){}

Scene* lastScene;

void Editor::PlayScene(){
    Assert(SceneManager::Get().GetActiveScene() != nullptr);
    if(SceneManager::Get().GetActiveScene()->Running()) return;

    BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
    Assert(renderPipeline != nullptr);
    Assert(dynamic_cast<StandRenderPipeline*>(renderPipeline));

    /*Scene* scene = SceneManager::Get().activeScene();
    if(_curScenePath.empty()) scene->Save("res/temp.scene");
    scene->Start();*/

    UnselectAll();
    lastScene = SceneManager::Get().GetActiveScene();
    renderPipeline->SetOverrideFrameBuffer(nullptr);
    Scene* s = Scene::Copy(lastScene);
    SceneManager::Get().SetActiveScene(s);
    s->Start();
}

void Editor::StopScene(){
    Assert(SceneManager::Get().GetActiveScene() != nullptr);
    if(SceneManager::Get().GetActiveScene()->Running() == false) return;

    BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
    Assert(renderPipeline != nullptr);

    //if(SceneManager::Get().activeScene() != nullptr && SceneManager::Get().activeScene()->running() == false) return;
    
    /*Scene* scene = SceneManager::Get().NewScene();
    scene->Load(_curScenePath.empty() ? "res/temp.scene" : _curScenePath.c_str());
    UnselectAll();*/

    UnselectAll();
    renderPipeline->SetOverrideFrameBuffer(nullptr);
    delete SceneManager::Get().GetActiveScene();
    SceneManager::Get().SetActiveScene(lastScene);
}

void Editor::NewScene(){
    if(SceneManager::Get().GetActiveScene()->Running()) return;

    BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
    Assert(renderPipeline != nullptr);

    renderPipeline->SetOverrideFrameBuffer(nullptr);
    Scene* scene = SceneManager::Get().NewScene();

    Entity e = scene->AddEntity("Enviroment");
    e.AddComponent<EnvironmentComponent>();

    UnselectAll();
    curScenePath = "";
}   

void Editor::OpenScene(){
    Assert(SceneManager::Get().GetActiveScene() != nullptr);
    if(SceneManager::Get().GetActiveScene()->Running()) return;

    BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
    Assert(renderPipeline != nullptr);

    std::string path = FileDialogs::OpenFile(""); 
    if(path.empty() == false){
        renderPipeline->SetOverrideFrameBuffer(nullptr);
        Scene* scene = SceneManager::Get().NewScene();

        scene->Load(path.c_str());
        curScenePath = scene->Path();
    }
    UnselectAll();
}

void Editor::SaveAsScene(){
    if(SceneManager::Get().GetActiveScene()->Running()) return;

    std::string path = FileDialogs::SaveFile("");
    if(path.empty() == false){
        Scene* scene = SceneManager::Get().GetActiveScene();
        scene->Save(path.c_str());
        curScenePath = path;
    } 
}

void Editor::HandleShotcuts(){
    if(Input::IsKeyDown(KeyCode::F1)) PlayScene();
    if(Input::IsKeyDown(KeyCode::F2)) StopScene();

    if(SceneManager::Get().GetActiveScene()->Running()) return;

    if(Input::IsKeyDown(KeyCode::Q)) gizmoType = Editor::GizmosType::None;
    if(Input::IsKeyDown(KeyCode::W)) gizmoType = Editor::GizmosType::Translation;
    if(Input::IsKeyDown(KeyCode::E)) gizmoType = Editor::GizmosType::Rotation;
    if(Input::IsKeyDown(KeyCode::R)) gizmoType = Editor::GizmosType::Scale;
}

void Editor::DrawMainPanel(){
    OD_PROFILE_SCOPE("Editor::DrawMainPanel");

    ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    DrawMainMenuBar();

    static bool workspace = true;
    static bool code = true;
    bool isCode;

    ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration;
    float height = ImGui::GetFrameHeight();
    if(ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flags)){
        if(ImGui::BeginMenuBar()){
            bool sceneRunning = SceneManager::Get().GetActiveScene()->Running();
            ImGui::Text("Scene Status: %s", sceneRunning ? "Running" : "Idle");
            ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
            ImGui::Button("Test", ImVec2(0, height-1));

            static int e = 0;
            ImGui::RadioButton("radio a", &e, 0); ImGui::SameLine();
            ImGui::RadioButton("radio b", &e, 1); ImGui::SameLine();
            ImGui::RadioButton("radio c", &e, 2);
            
            ImGui::BeginTabBar("BeginTabBar");
            
            if(ImGui::BeginTabItem("Workspace", &workspace, ImGuiTabItemFlags_NoCloseButton)){
                //DrawMainWorkspace();
                isCode = false;
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Workspace2", &code)){
                isCode = true;
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
    
    if(isCode == false){
        DrawMainWorkspace();
    }
}

void Editor::DrawMainMenuBar(){
    OD_PROFILE_SCOPE("Editor::DrawMainMenuBar");

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

        if(ImGui::BeginMenu("MainWorkspace")){
            for(auto i: mainWorkspace.panels){
                ImGui::MenuItem(i->name.c_str(), "", &i->show);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Editor::DrawMainWorkspace(){
    OD_PROFILE_SCOPE("Editor::DrawMainWorkspace");

    mainWorkspace.SetScene(SceneManager::Get().GetActiveScene());
    mainWorkspace.SetEditor(this);
    mainWorkspace.OnGui();
    
    /*
    sceneHierarchyPanel.SetScene(SceneManager::Get().GetActiveScene());
    sceneHierarchyPanel.SetEditor(this);

    inspectorPanel.SetScene(SceneManager::Get().GetActiveScene());
    inspectorPanel.SetEditor(this);
    
    sceneHierarchyPanel.OnGui();
    contentBrowserPanel.OnGui();
    inspectorPanel.OnGui();

    //ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Renderer Stats");
    ImGui::Text("DrawCalls: %d", Graphics::drawCalls);
    ImGui::Text("Vertices: %dk", Graphics::vertices / 1000);
    ImGui::Text("Tris: %dk", Graphics::tris / 1000);
    ImGui::End();

    if(ImGui::Begin("Profile")){
        for(auto i: Instrumentor::Get().results()){ 
            float durration = (i.end - i.start) * 0.001f;
            ImGui::Text("%s: %.3f.ms", i.name, durration);
        }
        Instrumentor::Get().results().clear();
    }
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::Begin("Viewport");

    auto& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    viewportSize.x = viewportPanelSize.x;
    viewportSize.y = viewportPanelSize.y;

    auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
    auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
    auto viewportOffset = ImGui::GetWindowPos();
    //ImVec2 m_ViewportBounds[2];
    viewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
    viewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

    uint32_t textureId = framebuffer->ColorAttachmentId(0);

    auto[mx, my] = ImGui::GetMousePos();
    mx -= viewportBounds[0].x;
    my -= viewportBounds[0].y;
    Vector2 viewportSize = Vector2(viewportBounds[1].x, viewportBounds[1].y) - Vector2(viewportBounds[0].x, viewportBounds[0].y);
    my = viewportSize.y - my;
    int mouseX = (int)mx;
    int mouseY = (int)my;

    BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
    Assert(renderPipeline != nullptr);

    //LogInfo("screen_pos x: %d y: %d", mouseX, mouseY);
    
    if(renderPipeline->FinalColor()->IsValid()){
        //SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->objectsId()->Bind();
        //LogInfo("ReadPixel(1): %d",SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->objectsId()->ReadPixel(0, mouseX, mouseY));
        //SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->objectsId()->Unbind();

        //textureId = SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->finalColor()->ColorAttachmentId(1);
        //SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->finalColor()->ColorAttachmentId(0);
    }

    ImVec2 imagePos = ImGui::GetCursorPos();
    
    //ImGui::Image((ImTextureID)textureId, ImVec2(viewportPanelSize.x, viewportPanelSize.y), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::Image((void*)(uint64_t)textureId, ImVec2(viewportPanelSize.x, viewportPanelSize.y), ImVec2(0, 1), ImVec2(1, 0));

    if(ImGui::BeginDragDropTarget()){
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ContentBrowserPanelFile");

        if(payload != nullptr){
            std::filesystem::path* path = (std::filesystem::path*)payload->Data;
            LogInfo("%s", path->string().c_str());
        }
        
        ImGui::EndDragDropTarget();
    }

    ImGui::SetCursorPos(ImVec2(imagePos.x + 5, imagePos.y + 5));
    ImGui::BeginGroup();
    ImGui::SmallButton("X"); 
    ImGui::SmallButton("Y");
    ImGui::SmallButton("Z");
    ImGui::EndGroup();

    //_framebuffer->Resize((int)viewportPanelSize.x, (int)viewportPanelSize.y);

    DrawGizmos();

    ImGui::End();
    ImGui::PopStyleVar();
    //io.ConfigWindowsMoveFromTitleBarOnly = false;
    */
}

void Editor::DrawGizmos(){
    if(selectionEntity.IsValid() == false) return;
    if(gizmoType == Editor::GizmosType::None) return;

    Camera cam = editorCam.cam;

    if(SceneManager::Get().GetActiveScene()->Running()){
        Entity camE = SceneManager::Get().GetActiveScene()->GetMainCamera2();
        if(camE.IsValid() == false) return;

        CameraComponent& cameraComponent = camE.GetComponent<CameraComponent>();
        cam = cameraComponent.GetCamera();
    }
    
    ImGuizmo::Enable(true);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    //ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

    ImGuizmo::SetRect(viewportBounds[0].x, viewportBounds[0].y, viewportBounds[1].x - viewportBounds[0].x, viewportBounds[1].y - viewportBounds[0].y);
    //ImGuiIO& io = ImGui::GetIO();
    //ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    Matrix4 view = cam.view;
    Matrix4 projection = cam.projection;

    TransformComponent& tc = selectionEntity.GetComponent<TransformComponent>();
    Matrix4 trans = tc.GlobalModelMatrix();

    bool snap = Input::IsKey(KeyCode::Control);
    float snapValue = 1;
    if(gizmoType == Editor::GizmosType::Rotation) snapValue = 45;

    float snapValues[3] = {snapValue, snapValue, snapValue};

    ImGuizmo::OPERATION _gizmoType = ImGuizmo::OPERATION::TRANSLATE;
    if(gizmoType == Editor::GizmosType::Rotation) _gizmoType = ImGuizmo::OPERATION::ROTATE;
    if(gizmoType == Editor::GizmosType::Scale) _gizmoType = ImGuizmo::OPERATION::SCALE;

    ImGuizmo::Manipulate(
        Mathf::Raw(view),
        Mathf::Raw(projection),
        _gizmoType, 
        ImGuizmo::LOCAL,
        Mathf::Raw(trans),
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

        if(gizmoType == Editor::GizmosType::Translation) tc.Position(t);
        if(gizmoType == Editor::GizmosType::Rotation) tc.Rotation(r);
        if(gizmoType == Editor::GizmosType::Scale) tc.LocalScale(s);
    }
}

}