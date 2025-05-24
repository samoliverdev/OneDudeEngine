#include "ViewportPanel.h"
#include "OD/Core/Input.h"
#include "OD/Editor/Editor.h"
#include "OD/RenderPipeline/StandRenderPipeline.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"

namespace OD{

ViewportPanel::ViewportPanel(){
    name = "ViewportPanel";
    show = true;
}

void ViewportPanel::OnGui(){
    if(editor == nullptr){
        LogError("ViewportPanel:EditorNotAssigned");
        return;
    }

    bool sceneRunning = SceneManager::Get().GetActiveScene()->Running();

    void* textureId = editor->framebuffer->ColorAttachmentId(0);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(576, 680));
    ImGui::Begin("Viewport", (bool*)0, sceneRunning ? 0 : ImGuiWindowFlags_MenuBar);

    if(sceneRunning == false){
        ImGui::BeginMenuBar();

        /*ImGui::Button("Test");
        static int e = 0;
        ImGui::RadioButton("radio a", &e, 0);
        ImGui::RadioButton("radio b", &e, 1);
        ImGui::RadioButton("radio c", &e, 2);*/

        ImGui::Checkbox("Gizmos", &RenderContext::GetSettings().enableGizmos);
        ImGui::Checkbox("GizmosRuntime", &RenderContext::GetSettings().enableGizmosRuntime);
        ImGui::Checkbox("Wireframe", &RenderContext::GetSettings().enableWireframe);

        ImGui::Spacing();
        ImGui::Separator();

        if(ImGui::Button("Snap Settings")){
            ImVec2 buttonPos = ImGui::GetItemRectMin();
            ImVec2 buttonSize = ImGui::GetItemRectSize();
            ImGui::SetNextWindowPos(ImVec2(buttonPos.x, buttonPos.y + buttonSize.y));
            ImGui::OpenPopup("Snap Settings Popup");
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8)); // 8 pixels on all sides
        if(ImGui::BeginPopup("Snap Settings Popup")){
            //ImGui::Dummy(ImVec2(0, 4)); // small top padding

            ImGui::Text("Grid Snap Settings");
            ImGui::Checkbox("Enable", &editor->snapSettings.enable);
            ImGui::InputFloat("Grid Size", &editor->snapSettings.posGridSize);
            ImGui::InputFloat("Snap Angle", &editor->snapSettings.rotSnapAngle);

            // Optional: Clamp values if needed
            if(editor->snapSettings.posGridSize < 0.001f) editor->snapSettings.posGridSize = 0.001f;
            if(editor->snapSettings.rotSnapAngle < 0.1f) editor->snapSettings.rotSnapAngle = 0.1f;

            //ImGui::Dummy(ImVec2(0, 4)); // small bottom padding
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        float labelWidth = ImGui::CalcTextSize("Center").x;
        ImGui::PushItemWidth(labelWidth + 30.0f); // Adicione um pouco para o botão de dropdown
        ImGui::DrawEnumCombo<Editor::GizmoPivotMode>("##pivotMode", &editor->pivotMode);
        ImGui::PopItemWidth();

        labelWidth = ImGui::CalcTextSize("Global").x;
        ImGui::PushItemWidth(labelWidth + 30.0f); // Adicione um pouco para o botão de dropdown
        ImGui::DrawEnumCombo<Editor::GizmoSpace>("##gizmoSpace", &editor->gizmoSpace);
        ImGui::PopItemWidth();

        /*static const char* items[]{"One","Two","three"};
        static int Selecteditem = 0;
        if(ImGui::Combo("MyCombo", &Selecteditem, items, IM_ARRAYSIZE(items))){
            // Here event is fired
        }*/

        /*if(ImGui::RadioButton("Pivot", editor->pivotMode == Editor::GizmoPivotMode::Pivot))
            editor->pivotMode = Editor::GizmoPivotMode::Pivot;
        if(ImGui::RadioButton("Center", editor->pivotMode == Editor::GizmoPivotMode::Center))
            editor->pivotMode = Editor::GizmoPivotMode::Center;*/

        /*if(Selecteditem == 1){
            StandRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<StandRenderPipeline>();
            textureId = renderPipeline->GetCameraRenderer().GetShadows().GetDirectionalShadowAtlas()->DepthAttachmentId();
        }*/

        ImGui::EndMenuBar();
    }

    auto& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    if(viewportPanelSize.x > 0) editor->viewportSize.x = viewportPanelSize.x;
    if(viewportPanelSize.y > 0) editor->viewportSize.y = viewportPanelSize.y;

    auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
    auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
    auto viewportOffset = ImGui::GetWindowPos();
    //ImVec2 m_ViewportBounds[2];
    editor->viewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
    editor->viewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

    

    auto[mx, my] = ImGui::GetMousePos();
    mx -= editor->viewportBounds[0].x;
    my -= editor->viewportBounds[0].y;
    Vector2 viewportSize = Vector2(editor->viewportBounds[1].x, editor->viewportBounds[1].y) -
        Vector2(editor->viewportBounds[0].x, editor->viewportBounds[0].y);
    
    my = editor->viewportSize.y - my;
    int mouseX = (int)mx;
    int mouseY = (int)my;

    BaseRenderPipeline* renderPipeline = SceneManager::Get().GetActiveScene()->GetSystemDynamic<BaseRenderPipeline>();
    Assert(renderPipeline != nullptr);

    if(Input::IsMouseButtonDown(MouseButton::Left) && editor->gizmoInteractionState.isOver == false){
        int entityId = renderPipeline->ReadEntityId(mouseX, mouseY) - 1;
    
        ImVec2 _mousePos = ImGui::GetMousePos();
        bool insideViewport =
        _mousePos.x >= editor->viewportBounds[0].x && _mousePos.x <= editor->viewportBounds[1].x &&
        _mousePos.y >= editor->viewportBounds[0].y && _mousePos.y <= editor->viewportBounds[1].y;

        if(insideViewport){
            LogInfo("ReadPixel(1): %d", entityId);

            if(entityId >= 0){
                bool holdMultSelection = Input::IsKey(KeyCode::LShift);

                if(holdMultSelection){
                    editor->AddSelectionEntity((Entity)entityId);
                } else {    
                    editor->SetSelectionEntity((Entity)entityId);
                }
            } else {
                editor->UnselectAll();
            }
        }
    }

    //LogInfo("screen_pos x: %d y: %d", mouseX, mouseY);
    
    if(renderPipeline->FinalColor()->IsValid()){
        //SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->objectsId()->Bind();
        //LogInfo("ReadPixel(1): %d",SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->objectsId()->ReadPixel(0, mouseX, mouseY));
        //SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->objectsId()->Unbind();

        //textureId = SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->finalColor()->ColorAttachmentId(1);
        //SceneManager::Get().activeScene()->GetSystem<StandRendererSystem>()->finalColor()->ColorAttachmentId(0);
    }
    
    ImVec2 imagePos = ImGui::GetCursorPos();
    
    ImGui::Image(textureId, ImVec2(viewportPanelSize.x, viewportPanelSize.y), ImVec2(0, 1), ImVec2(1, 0));
    //ImGui::Image((void*)(uint64_t)textureId, ImVec2(viewportPanelSize.x, viewportPanelSize.y), ImVec2(0, 1), ImVec2(1, 0));

    if(ImGui::BeginDragDropTarget()){
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ContentBrowserPanelFile");

        if(payload != nullptr){
            std::filesystem::path* path = (std::filesystem::path*)payload->Data;
            LogInfo("%s", path->string().c_str());
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
                Ref<Model> model = AssetManager::Get().LoadAsset<Model>(path->string());
                Entity mEntity = scene->AddEntity(getFileNameWithoutExtension(*path));
                ModelRendererComponent& mRenderer = scene->AddComponent<ModelRendererComponent>(mEntity);
                mRenderer.SetModel(model);
            }

            LogInfo("Reciving File: %s", path->string().c_str());
        }
        
        ImGui::EndDragDropTarget();
    }

    if(sceneRunning == false){
        ImGui::SetCursorPos(ImVec2(imagePos.x + 5, imagePos.y + 25));
        ImGui::BeginGroup();
        ImGui::SmallButton("X"); 
        ImGui::SmallButton("Y");
        ImGui::SmallButton("Z");
        ImGui::EndGroup();
    }

    //_framebuffer->Resize((int)viewportPanelSize.x, (int)viewportPanelSize.y);

    editor->DrawGizmos();

    ImGui::End();
    ImGui::PopStyleVar();
}

}