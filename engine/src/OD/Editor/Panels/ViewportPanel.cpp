#include "ViewportPanel.h"
#include "OD/Editor/Editor.h"

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
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::Begin("Viewport");

    auto& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    editor->viewportSize.x = viewportPanelSize.x;
    editor->viewportSize.y = viewportPanelSize.y;

    auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
    auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
    auto viewportOffset = ImGui::GetWindowPos();
    //ImVec2 m_ViewportBounds[2];
    editor->viewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
    editor->viewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

    uint32_t textureId = editor->framebuffer->ColorAttachmentId(0);

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

    editor->DrawGizmos();

    ImGui::End();
    ImGui::PopStyleVar();
}

}