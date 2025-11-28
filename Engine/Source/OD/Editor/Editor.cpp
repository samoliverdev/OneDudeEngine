#include "Editor.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Undo.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Scene/BaseRenderPipeline.h"
#include "OD/Core/ImGui.h"
#include "OD/Platform/Platform.h"
#include "OD/RenderPipeline/CameraComponent.h"
#include "OD/RenderPipeline/StandRenderPipeline.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/Core/Input.h"
#include <imgui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>
#include "OD/Core/Application.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Utils/File.h"
#include <filesystem>
#include <iostream>
#include <fstream>

namespace OD{

struct MenuNode{
    std::function<void()> command = nullptr;
    std::unordered_map<std::string, MenuNode> child;
};
std::unordered_map<std::string, MenuNode> menuNodes;

std::vector<std::string> split(const std::string& s, char delim){
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;

    while(getline(ss, item, delim)){
        result.push_back(item);
    }

    return result;
}

//Editor* Editor::instance = nullptr;
Editor* instance = nullptr;

Editor* Editor::Get(){ return instance; }

void Editor::AddMenuCommand(const std::string path, std::function<void()> command){
    auto p = split(path, '/');
    MenuNode* curNode = nullptr;

    for(auto i: p){
        if(curNode == nullptr){
            if(menuNodes.count(i)){
                curNode = &menuNodes[i];
            } else {
                menuNodes[i] = MenuNode();
                curNode = &menuNodes[i];
            }
        } else {
            if(curNode->child.count(i)){
                curNode = &curNode->child[i];
            } else {
                curNode->child[i] = MenuNode();
                curNode = &curNode->child[i];
            }
        }
    }

    Assert(curNode != nullptr);
    curNode->command = command;
}

void DrawCustomMenu(const std::string& name, MenuNode& node){
    bool isLeaf = node.child.size() == 0;

    if(isLeaf == false){
        if(ImGui::BeginMenu(name.c_str())){
            for(auto i: node.child){
                //ImGui::MenuItem("tester");
                DrawCustomMenu(i.first, i.second);
            }
            ImGui::EndMenu();
        }
    } else {
        if(ImGui::MenuItem(name.c_str()) && node.command != nullptr) node.command();
    }
}

Entity Editor::GetTargetSelected(Entity entity){
    if(entity == EntityNull) return entity;

    Assert(SceneManager::Get().GetActiveScene() != nullptr);
    InfoComponent& info = SceneManager::Get().GetActiveScene()->GetComponent<InfoComponent>(entity);
    TransformComponent& trans = SceneManager::Get().GetActiveScene()->GetComponent<TransformComponent>(entity);

    if(info.Type() != EntityType::Stand && trans.HasParent()){
        InfoComponent& infoParent = SceneManager::Get().GetActiveScene()->GetComponent<InfoComponent>(trans.Parent());
        if(infoParent.Type() == EntityType::Stand) return entity;

        return GetTargetSelected(trans.Parent());
    }

    return entity;
}

inline void AlignToViewAABB(Transform& cameraTransform, const AABB& aabb, float verticalFOVDegrees = 60.0f, float aspectRatio = 16.0f/9.0f, Vector3 viewDirection = Vector3Back, Vector3 upDirection = Vector3Up) {
    #undef max
    // 1. Obter o centro do AABB
    Vector3 center = aabb.center;
    Vector3 extents = aabb.extents;

    // 2. Calcular o raio da esfera que engloba o AABB
    float radius = math::length(extents);

    // 3. Converter FOV vertical de graus para radianos
    float verticalFOVRadians = Mathf::Deg2Rad(verticalFOVDegrees);

    // 4. Calcular a distância necessária para caber o AABB na vertical do frustum
    float distanceY = radius / std::sin(verticalFOVRadians / 2.0f);

    // 5. Calcular a distância horizontal necessária (com base na proporção da tela)
    float horizontalFOVRadians = 2.0f * std::atan(std::tan(verticalFOVRadians / 2.0f) * aspectRatio);
    float distanceX = radius / std::sin(horizontalFOVRadians / 2.0f);

    // 6. Usar a maior distância para garantir que o AABB esteja completamente visível
    float distance = std::max(distanceX, distanceY);

    // 7. Calcular a nova posição da câmera
    Vector3 cameraPos = center - viewDirection * distance;

    // 8. Aplicar nova posição e rotação para olhar para o centro do AABB
    cameraTransform.Position(cameraPos);
    cameraTransform.LookAt(center, upDirection);
}

inline void AlignCameraToAABB_Isometric(Transform& cameraTransform, const AABB& bounds, float verticalFovDeg, float aspectRatio, float borderScale = 1.2f, float yawDeg = 45.0f, float pitchDeg = 30.0f) {
    using namespace math;

    // Centro e tamanho da AABB
    Vector3 center = bounds.center;
    Vector3 extents = bounds.extents;

    // Converte ângulos para radianos
    float yawRad   = Mathf::Deg2Rad(yawDeg);
    float pitchRad = Mathf::Deg2Rad(pitchDeg);

    // Direção da câmera: ajustada para vir de cima olhando para baixo
    Vector3 viewDir = normalize(Vector3(
        std::cos(pitchRad) * std::sin(yawRad),  // X
       -std::sin(pitchRad),                     // Y (NEGATIVO para vir de cima!)
        std::cos(pitchRad) * std::cos(yawRad)   // Z
    ));

    // Calcula o raio da AABB
    float radius = length(extents);

    // Calcula a distância ideal com base no FOV
    float halfFovRad = Mathf::Deg2Rad(verticalFovDeg * 0.5f);
    float viewDistance = (radius * borderScale) / std::tan(halfFovRad);

    // Define a posição da câmera deslocada na direção oposta ao viewDir
    Vector3 cameraPos = center - viewDir * viewDistance;

    // Define a posição e rotação da câmera
    cameraTransform.Position(cameraPos);
    cameraTransform.LookAt(center);
}

int Editor::ExecutionSortPriority(){ 
    return 10000; 
};

void Editor::OnInit(){
    ImGuiLayer::SetCleanAll(true);

    FrameBufferSpecification framebufferSpecification = {Application::ScreenWidth(), Application::ScreenHeight()};
    framebufferSpecification.colorAttachments = {{FramebufferTextureFormat::RGB16F}};
    framebufferSpecification.depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};
    framebuffer = new Framebuffer(framebufferSpecification);
    //framebuffer = new Framebuffer(FramebufferType::Stand, Application::ScreenWidth(), Application::ScreenHeight());
    framebuffer->Invalidate();

    assetPreviewFramebuffer = new Framebuffer(framebufferSpecification);
    assetPreviewFramebuffer->Invalidate();

    viewportSize.x = framebuffer->Width();
    viewportSize.y = framebuffer->Height();

    instance = this;

    editorCam.OnStart();
    editorCam.moveSpeed = 60;
    editorCam.fastMoveSpeed = 300;

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
    mainWorkspace.AddPanel(&globalSettingsPanel);
    mainWorkspace.AddPanel(&runtimeInfoPanel);

    std::ifstream is("Editor.Save");
    if(is.fail() == false){
        cereal::JSONInputArchive archive{is};
        archive(cereal::make_nvp("Editor", *this));
    }

    assetPreviewScene = new Scene();
    BaseRenderPipeline* renderP = assetPreviewScene->GetSystemDynamic<BaseRenderPipeline>();
    renderP->SetOverrideCamera(&assetPrevieweCam.cam, assetPrevieweCam.transform);
    renderP->SetOverrideFrameBuffer(assetPreviewFramebuffer);
    assetPreviewFramebuffer->Resize(400, 400);

    assetPrevieweCam.transform.Position({0, 1, 5});

    assetPreviewEntity = assetPreviewScene->AddEntity("ModelPreview");
    ModelRendererComponent& model = assetPreviewScene->AddComponent<ModelRendererComponent>(assetPreviewEntity);
    model.SetModel(AssetManager::Get().LoadAsset<Model>("Engine/Models/Cube.obj"));

    //OD::AlignToViewAABB(assetPrevieweCam.transform, model.GetAABB(), 45.0f, 16.0f/9.0f);
    OD::AlignCameraToAABB_Isometric(
        assetPrevieweCam.transform, model.GetAABB(), 60.0f, 16.0f / 9.0f, 1.5f
    );
    assetPrevieweCam.target = model.GetAABB().center;
    assetPrevieweCam.OnStart();
}

void Editor::SetModelAssetPreview(Ref<Model> m){
    if(lastModelAssetPreview != m){
        if(assetPreviewEntity != EntityNull) assetPreviewScene->DestroyEntity(assetPreviewEntity);

        assetPreviewEntity = assetPreviewScene->AddEntity("Asset Preview");
        ModelRendererComponent& model = assetPreviewScene->AddOrGetComponent<ModelRendererComponent>(assetPreviewEntity);
        model.SetModel(m);

        OD::AlignCameraToAABB_Isometric(assetPrevieweCam.transform, model.GetAABB(), 60.0f, 16.0f / 9.0f, 1.5f);
        assetPrevieweCam.target = model.GetAABB().center;
        assetPrevieweCam.OnStart();
    }

    lastModelAssetPreview = m;
    lastPrefabAssetPreview = nullptr;
}

void Editor::SetModelAssetPreview(const std::string& path){
    auto m = AssetManager::Get().LoadAsset<Model>(path);

    if(lastModelAssetPreview != m){
        if(assetPreviewEntity != EntityNull) assetPreviewScene->DestroyEntity(assetPreviewEntity);

        assetPreviewEntity = assetPreviewScene->AddEntity("Asset Preview");
        ModelRendererComponent& model = assetPreviewScene->AddOrGetComponent<ModelRendererComponent>(assetPreviewEntity);
        model.SetModel(m);

        OD::AlignCameraToAABB_Isometric(assetPrevieweCam.transform, model.GetAABB(), 60.0f, 16.0f / 9.0f, 1.5f);
        assetPrevieweCam.target = model.GetAABB().center;
        assetPrevieweCam.OnStart();
    }

    lastModelAssetPreview = m;
    lastPrefabAssetPreview = nullptr;
}

void Editor::SetPrefabAssetPreview(Ref<Prefab> prefab){

}

void Editor::SetPrefabAssetPreview(const std::string& path){
    auto prefab = AssetManager::Get().LoadAsset<Prefab>(path);

    if(lastPrefabAssetPreview != prefab){
        if(assetPreviewEntity != EntityNull) assetPreviewScene->DestroyEntity(assetPreviewEntity);

        //assetPreviewEntity = assetPreviewScene->InstantiatePrefab(prefab->Path().c_str());
        assetPreviewEntity = assetPreviewScene->InstantiatePrefab(*prefab);

        AABB aabb({0, 0, 0}, 10, 10, 10);

        OD::AlignCameraToAABB_Isometric(assetPrevieweCam.transform, aabb, 60.0f, 16.0f / 9.0f, 1.5f);
        assetPrevieweCam.target = aabb.center;
        assetPrevieweCam.OnStart();
    }

    lastPrefabAssetPreview = prefab;
    lastModelAssetPreview = nullptr;
}

void Editor::OnExit(){
    delete assetPreviewScene;
    delete assetPreviewFramebuffer;
    delete framebuffer;

    //LogInfo("Edito::OnExit");

    std::ofstream os("Editor.Save");
    cereal::JSONOutputArchive archive{os};
    archive(cereal::make_nvp("Editor", *this));
}

void Editor::OnUpdate(float deltaTime){
    OD_PROFILE_SCOPE("Editor::OnUpdate");

    Assert(SceneManager::Get().GetActiveScene() != nullptr);
    if(SceneManager::Get().GetActiveScene() == nullptr) return;
    
    if(Input::IsKeyDown(KeyCode::F5)) open = !open;

    if(Input::IsKey(KeyCode::Control) && Input::IsKeyDown(KeyCode::Z)){
        UndoManager::Get().Undo();
    }
    if(Input::IsKey(KeyCode::Control) && Input::IsKeyDown(KeyCode::Y)){
        UndoManager::Get().Redo();
    }
    if(Input::IsKey(KeyCode::Control) && Input::IsKeyDown(KeyCode::D)){
        auto* scene = SceneManager::Get().GetActiveScene();
        if(/*scene->IsValid(selectionEntity)*/ _selectedEntities.size() > 0){
            //scene->DuplicateEntity(selectionEntity);

            std::vector<Entity> newEntities;
            for(auto& i: _selectedEntities){
                newEntities.push_back(scene->DuplicateEntity(i));
            }
            UnselectAll();
            for(auto& i: newEntities){
                AddSelectionEntity(i);
            }
        }
    }

    BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
    Assert(renderPipeline != nullptr);
    //Assert(dynamic_cast<StandRenderPipeline2*>(renderPipeline));

    if(SceneManager::Get().GetActiveScene()->Running()){
        renderPipeline->SetOverrideCamera(nullptr, Transform());
    } else {
        float width = viewportSize.x;
        float height = viewportSize.y;
        
        if(drawSceneToCustomFramebuffer){
            if(open == false){
                width = Application::ScreenWidth();
                height = Application::ScreenHeight();
            }
        } else {
            width = Application::ScreenWidth();
            height = Application::ScreenHeight();
        }

        editorCam.OnUpdate();
        editorCam.cam.isDebug = true;
        editorCam.cam.SetPerspective(45, 0.1f, 20000.0f, width, height);
        editorCam.cam.viewPos = editorCam.transform.Position();
        editorCam.cam.view = math::inverse(editorCam.transform.GetModelMatrix());
        //editorCam.cam.frustum = CreateFrustumFromCamera(editorCam.transform, width / height, Mathf::Deg2Rad(45), 0.1f, 2000.0f);
        editorCam.cam.frustum = CreateFrustumFromMatrix2(math::transpose( editorCam.cam.projection * editorCam.cam.view ));
        renderPipeline->SetOverrideCamera(&editorCam.cam, editorCam.transform);
    }

    if(drawSceneToCustomFramebuffer){
        if(open == false){
            renderPipeline->SetOverrideFrameBuffer(nullptr);
            ImGuiLayer::SetCleanAll(false);
        } else{
            ImGuiLayer::SetCleanAll(true);
            renderPipeline->SetOverrideFrameBuffer(framebuffer);
            framebuffer->Resize(viewportSize.x, viewportSize.y);
        }
    } else {
        renderPipeline->SetOverrideFrameBuffer(nullptr);
        ImGuiLayer::SetCleanAll(false);
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

    assetPrevieweCam.OnUpdate();
    assetPrevieweCam.cam.isDebug = true;
    assetPrevieweCam.cam.SetPerspective(45, 0.1f, 20000.0f, 400, 400);
    assetPrevieweCam.cam.viewPos = assetPrevieweCam.transform.Position();
    assetPrevieweCam.cam.view = math::inverse(assetPrevieweCam.transform.GetModelMatrix());
    assetPrevieweCam.cam.frustum = CreateFrustumFromMatrix2(
        math::transpose(assetPrevieweCam.cam.projection * assetPrevieweCam.cam.view)
    );
    assetPreviewScene->Update();
    assetPreviewScene->Draw();
}

void Editor::OnRender(float deltaTime){
    //assetPreviewScene->Draw();
}

void Editor::OnGUI(){
    OD_PROFILE_SCOPE("Editor::OnGUI");

    //OD::ImGuiLayer::SetCleanAll(false);

    if(open == false) return;

    DrawMainPanel();

    //static bool show;
    //ImGui::ShowDemoWindow(&show);
}

void Editor::OnResize(int width, int height){}

Scene* lastScene;

void Editor::PlayScene(){
    Assert(SceneManager::Get().GetActiveScene() != nullptr);
    if(SceneManager::Get().GetActiveScene()->Running()) return;

    BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
    Assert(renderPipeline != nullptr);
    //Assert(dynamic_cast<StandRenderPipeline*>(renderPipeline));

    /*Scene* scene = SceneManager::Get().activeScene();
    if(_curScenePath.empty()) scene->Save("res/temp.scene");
    scene->Start();*/

    //UnselectAll();
    lastScene = SceneManager::Get().GetActiveScene();
    renderPipeline->SetOverrideFrameBuffer(nullptr);
    //Scene* s = Scene::Copy(lastScene);
    Scene* s = new Scene(*lastScene);
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

    Platform::SetCursorState(CursorState::Normal);
}

void Editor::NewScene(){
    if(SceneManager::Get().GetActiveScene() != nullptr && SceneManager::Get().GetActiveScene()->Running()) return;

    /*if(SceneManager::Get().GetActiveScene() != nullptr){
        BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
        Assert(renderPipeline != nullptr);
        renderPipeline->SetOverrideFrameBuffer(nullptr);
    }*/

    Scene* scene = SceneManager::Get().NewScene();

    Entity e = scene->AddEntity("Enviroment");
    scene->AddComponent<EnvironmentComponent>(e);

    UnselectAll();
    curScenePath = "";
}   

void Editor::OpenScene(){
    Assert(SceneManager::Get().GetActiveScene() != nullptr);
    if(SceneManager::Get().GetActiveScene()->Running()) return;

    BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
    Assert(renderPipeline != nullptr);

    std::string path = Platform::OpenFile("*.scene"); 
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

    std::string path = Platform::SaveFile("*.scene");
    if(path.empty() == false){
        Scene* scene = SceneManager::Get().GetActiveScene();
        scene->Save(path.c_str(), EntityNull);
        curScenePath = path;
    } 
}

void Editor::HandleShotcuts(){
    if(Input::IsKeyDown(KeyCode::F1)) PlayScene();
    if(Input::IsKeyDown(KeyCode::F2)) StopScene();

    //if(SceneManager::Get().GetActiveScene()->Running()) return;

    if(Input::IsKeyDown(KeyCode::Q)) gizmoType = Editor::GizmosType::None;
    if(Input::IsKeyDown(KeyCode::W)) gizmoType = Editor::GizmosType::Translation;
    if(Input::IsKeyDown(KeyCode::E)) gizmoType = Editor::GizmosType::Rotation;
    if(Input::IsKeyDown(KeyCode::R)) gizmoType = Editor::GizmosType::Scale;
}

void Editor::DrawMainPanel(){
    OD_PROFILE_SCOPE("Editor::DrawMainPanel");

    ImGui::DockSpaceOverViewport(ImGui::GetWindowDockID(), nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    DrawMainMenuBar();

    if(SceneManager::Get().GetActiveScene() == nullptr) NewScene();

    Assert(SceneManager::Get().GetActiveScene() != nullptr);
    if(SceneManager::Get().GetActiveScene() == nullptr) return;

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
            if(ImGui::MenuItem("OpenProject")){
                std::string path = Platform::OpenFolder(); 
                if(path.empty() == false){
                    auto exe = GetAbsExePath();
                    LogInfo("Project Dir: %s", path.c_str());
                    LogInfo("EditorPath: %s", exe.string().c_str());

                    std::string cmd = exe.string() + " " + path.c_str();
                    //ExecultCmd(cmd.c_str());
                    #ifdef _WIN32
                    WinExec(cmd.c_str(), SW_HIDE); 
                    #endif

                    Application::Quit();
                }
            }

            if(ImGui::MenuItem("New", "Ctrl+N")) NewScene();
            if(ImGui::MenuItem("Open...", "Ctrl+O")) OpenScene();
            if(ImGui::MenuItem("Save As", "Ctrl+Shift+S")) SaveAsScene();
            if(ImGui::MenuItem("Exit", "Alt+F4")) Application::Quit(); 
            if(ImGui::MenuItem("Undo", "Ctrl+Z")) UndoManager::Get().Undo();
            if(ImGui::MenuItem("Redo", "Ctrl+Y")) UndoManager::Get().Redo();

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Runtime")){
            if(ImGui::MenuItem("Play", "F1")) PlayScene();
            if(ImGui::MenuItem("Stop", "F2")) StopScene();
            ImGui::EndMenu();
        }

        for(auto& i: menuNodes){
            DrawCustomMenu(i.first, i.second);
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

inline float SnapValue(float value, float gridSize) {
    return std::round(value / gridSize) * gridSize;
}

/// Snaps a glm::vec3 to the grid
Vector3 SnapToGrid(const Vector3& position, float gridSize) {
    return Vector3(
        SnapValue(position.x, gridSize),
        SnapValue(position.y, gridSize),
        SnapValue(position.z, gridSize)
    );
}

/// Snaps a quaternion by converting to Euler angles, snapping, and converting back
Quaternion SnapToGrid(const Quaternion& rotation, float angleSnapDegrees) {
    Vector3 euler = math::eulerAngles(rotation); // radians
    euler = glm::degrees(euler); // convert to degrees

    euler.x = SnapValue(euler.x, angleSnapDegrees);
    euler.y = SnapValue(euler.y, angleSnapDegrees);
    euler.z = SnapValue(euler.z, angleSnapDegrees);

    return Quaternion(math::radians(euler)); // convert back to radians and then to quat
}

void Editor::DrawGizmos(){
    /*Assert(SceneManager::Get().GetActiveScene() != nullptr);
    Scene& scene = *SceneManager::Get().GetActiveScene();

    if(scene.IsValid(selectionEntity) == false) return;
    if(gizmoType == Editor::GizmosType::None) return;

    Camera cam = editorCam.cam;

    if(SceneManager::Get().GetActiveScene()->Running()){
        Entity camE = scene.GetMainCamera();
        if(scene.IsValid(camE) == false) return;

        CameraComponent& cameraComponent = scene.GetComponent<CameraComponent>(camE);
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

    TransformComponent& tc = scene.GetComponent<TransformComponent>(selectionEntity);
    Matrix4 trans = tc.GlobalModelMatrix();

    bool snap = Input::IsKey(KeyCode::Control);

    float snapValues[3] = {0, 0, 0};
    if(gizmoType == Editor::GizmosType::Rotation){
        snapValues[2] = snapValues[1] = snapValues[0] = snapSettings.rotSnapAngle;
    } else {
        snapValues[2] = snapValues[1] = snapValues[0] = snapSettings.posGridSize;
    }
    if(snapSettings.enable) snap = true;

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
    }*/


    Assert(SceneManager::Get().GetActiveScene() != nullptr);
    Scene& scene = *SceneManager::Get().GetActiveScene();

    // Exit if no entities are selected
    if(selectedEntities.empty()) return;
    if(gizmoType == Editor::GizmosType::None) return;

    Camera cam = editorCam.cam;

    if(SceneManager::Get().GetActiveScene()->Running()){
        Entity camE = scene.GetMainCamera();
        if(scene.IsValid(camE) == false) return;

        CameraComponent& cameraComponent = scene.GetComponent<CameraComponent>(camE);
        cam = cameraComponent.GetCamera();
    }

    ImGuizmo::Enable(true);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportBounds[0].x, viewportBounds[0].y, viewportBounds[1].x - viewportBounds[0].x, viewportBounds[1].y - viewportBounds[0].y);

    gizmoInteractionState.isUsing  = ImGuizmo::IsUsing();
    gizmoInteractionState.isOver = ImGuizmo::IsOver();

    if(gizmoInteractionState.isUsing && !gizmoInteractionState.active){
         // Início da manipulação
        gizmoInteractionState.active = true;
    }

    Matrix4 view = cam.view;
    Matrix4 projection = cam.projection;

    // Compute the gizmo's transform based on pivot mode
    Matrix4 gizmoTransform = glm::mat4(1.0f);
    Matrix4 originalGizmoTransform = glm::mat4(1.0f);
    glm::vec3 centerPosition(0.0f); // Store center position for rotation
    Entity pivotEntity; // Track the entity used for pivot rotation in Local mode
    bool hasValidEntity = false;
    int validCount = 0;

    if (pivotMode == GizmoPivotMode::Center && selectedEntities.size() > 1) {
        // Center mode: Use the average position, inherit rotation from the first valid entity
        glm::vec3 averagePosition(0.0f);
        for (const Entity& entity : selectedEntities) {
            if (scene.IsValid(entity)) {
                TransformComponent& tc = scene.GetComponent<TransformComponent>(entity);
                averagePosition += glm::vec3(tc.GlobalModelMatrix()[3]);
                if (validCount == 0) {
                    // Use the first valid entity's transform for rotation and scale
                    gizmoTransform = tc.GlobalModelMatrix();
                    pivotEntity = entity;
                }
                validCount++;
            }
        }
        if (validCount > 0) {
            averagePosition /= static_cast<float>(validCount);
            centerPosition = averagePosition; // Store for rotation
            gizmoTransform[3] = glm::vec4(averagePosition, 1.0f); // Set translation to average
            originalGizmoTransform = gizmoTransform; // Store original for delta calculation
            hasValidEntity = true;
        }
    } else {
        // Pivot mode: Use the transform of the first valid entity
        for (const Entity& entity : selectedEntities) {
            if (scene.IsValid(entity)) {
                TransformComponent& tc = scene.GetComponent<TransformComponent>(entity);
                gizmoTransform = tc.GlobalModelMatrix();
                originalGizmoTransform = gizmoTransform; // Store original for delta calculation
                centerPosition = glm::vec3(gizmoTransform[3]); // Use entity's position as center
                pivotEntity = entity;
                hasValidEntity = true;
                break;
            }
        }
    }

    // Skip if no valid entities
    if (!hasValidEntity) return;

    bool snap = Input::IsKey(KeyCode::Control);
    float snapValues[3] = {0, 0, 0};
    if (gizmoType == Editor::GizmosType::Rotation) {
        snapValues[2] = snapValues[1] = snapValues[0] = snapSettings.rotSnapAngle;
    } else {
        snapValues[2] = snapValues[1] = snapValues[0] = snapSettings.posGridSize;
    }
    if (snapSettings.enable) snap = true;

    ImGuizmo::OPERATION _gizmoType = ImGuizmo::OPERATION::TRANSLATE;
    if (gizmoType == Editor::GizmosType::Rotation) _gizmoType = ImGuizmo::OPERATION::ROTATE;
    if (gizmoType == Editor::GizmosType::Scale) _gizmoType = ImGuizmo::OPERATION::SCALE;

    ImGuizmo::MODE _gizmoMode = (gizmoSpace == GizmoSpace::Local) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    // Store a copy of the gizmo transform before manipulation
    Matrix4 gizmoTransformBefore = gizmoTransform;

    ImGuizmo::Manipulate(
        Mathf::Raw(view),
        Mathf::Raw(projection),
        _gizmoType,
        _gizmoMode,
        Mathf::Raw(gizmoTransform),
        nullptr,
        (snap ? snapValues : nullptr)
    );

    if(ImGuizmo::IsUsing()) {
        // Compute the delta transform
        glm::mat4 newGizmoTransform = (glm::mat4)gizmoTransform;
        glm::mat4 deltaTransform = newGizmoTransform * glm::inverse(originalGizmoTransform);

        // Apply the delta transform to all selected entities
        for(Entity entity : selectedEntities) {
            if(!scene.IsValid(entity)) continue;
            TransformComponent& tc = scene.GetComponent<TransformComponent>(entity);

            glm::mat4 entityTransform = tc.GlobalModelMatrix();
            glm::mat4 newEntityTransform;

            if (gizmoType == Editor::GizmosType::Rotation && pivotMode == GizmoPivotMode::Center && selectedEntities.size() > 1) {
                // Unity-like rotation around center pivot
                glm::vec3 scale, translation, skew;
                glm::quat deltaRotation;
                glm::vec4 perspective;
                glm::decompose(deltaTransform, scale, deltaRotation, translation, skew, perspective);

                // Translate entity relative to center, apply rotation, translate back
                glm::vec3 entityPos = glm::vec3(entityTransform[3]);
                glm::vec3 relativePos = entityPos - centerPosition;
                relativePos = deltaRotation * relativePos; // Rotate around center
                glm::vec3 newPos = centerPosition + relativePos;

                if(!transformChangesData.count(entity)) transformChangesData[entity].oldTrans = tc;
                // Update rotation (compose with existing rotation)
                glm::quat entityRotation = tc.Rotation();
                tc.Rotation(deltaRotation * entityRotation);
                tc.Position(newPos);
                transformChangesData[entity].newTrans = tc;
            } else if (gizmoSpace == GizmoSpace::Local && gizmoType != Editor::GizmosType::Scale) {
                // Local mode for translation or rotation
                if (gizmoType == Editor::GizmosType::Translation) {
                    glm::vec3 localTranslation = deltaTransform[3];

                    if(!transformChangesData.count(entity)) transformChangesData[entity].oldTrans = tc;
                    tc.Position(tc.Position() + localTranslation);
                    transformChangesData[entity].newTrans = tc;
                } else if (gizmoType == Editor::GizmosType::Rotation) {
                    glm::vec3 scale, translation, skew;
                    glm::quat deltaRotation;
                    glm::vec4 perspective;
                    glm::decompose(deltaTransform, scale, deltaRotation, translation, skew, perspective);
                    
                    if(!transformChangesData.count(entity)) transformChangesData[entity].oldTrans = tc;
                    tc.Rotation(deltaRotation * tc.Rotation());
                    transformChangesData[entity].newTrans = tc;
                }
            } else {
                // Global mode or scale: Apply the full transform
                newEntityTransform = deltaTransform * entityTransform;
                glm::vec3 newScale, newTranslation, skew;
                glm::quat newRotation;
                glm::vec4 perspective;
                glm::decompose(newEntityTransform, newScale, newRotation, newTranslation, skew, perspective);

                if(!transformChangesData.count(entity)) transformChangesData[entity].oldTrans = tc;
                if(gizmoType == Editor::GizmosType::Translation) tc.Position(newTranslation);
                if(gizmoType == Editor::GizmosType::Rotation) tc.Rotation(newRotation);
                if(gizmoType == Editor::GizmosType::Scale) tc.LocalScale(newScale);
                transformChangesData[entity].newTrans = tc;
            }
        }
    }

    if(!gizmoInteractionState.isUsing && gizmoInteractionState.active){
        // Fim da manipulação

        std::vector<TransformComponent> oldTrans;
        std::vector<TransformComponent> newTrans;
        std::vector<Entity> entities;
        for(auto& i: transformChangesData){
            if((Transform)i.second.oldTrans == (Transform)i.second.newTrans) continue;
            entities.push_back(i.first);
            oldTrans.push_back(i.second.oldTrans);
            newTrans.push_back(i.second.newTrans);
        }
        if(entities.size() > 0){
            UndoManager::Get().Execute(
                CreateScope<UndoValueComponentBatchCommand<TransformComponent>>(&scene, entities, oldTrans, newTrans)
            );
        }
        transformChangesData.clear();
        
        gizmoInteractionState.active = false;
    }

}

}