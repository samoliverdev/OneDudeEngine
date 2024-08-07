#include "EnvironmentComponent.h"
#include "OD/Serialization/CerealImGui.h"

namespace OD{

/*int ShadowQualityLookup[] = {512, 1024, 2048, 4096, 8192};
const char* ShadowQualityLookupNames[] = {"Low", "High", "VeryHigh", "Ultra", "VeryUltra"};

const char* AntiAliasingLookupNames[] = {"None", "MSAA"};

int MSAAQualityLookup[] = {2, 4, 8};
const char* MSAAQualityLookupNames[] = {"MSAA_2", "MSAA_4", "MSAA_8"};

const char* ColorCorrectionLookupNames[] = {"None", "ColorCorrection"};*/

void EnvironmentComponent::OnGui(Entity& e){
    EnvironmentComponent& environment = e.GetComponent<EnvironmentComponent>();

    ImGui::ColorEdit3("ambient", &environment.settings.ambient);
    ImGui::ColorEdit3("cleanColor", &environment.settings.cleanColor);
    
    ImGui::DrawAsset<Material>(std::string("sky"), environment.settings.sky);

    ImGui::Spacing();ImGui::Spacing();

    ImGui::DrawEnumCombo<ShadowQuality>("directionalshadowQuality", &environment.settings.directionalshadowQuality);
    ImGui::DrawEnumCombo<ShadowQuality>("othershadowQuality", &environment.settings.othershadowQuality);

    ImGui::DragFloat("shadowDistance", &environment.settings.shadowDistance);
    ImGui::DragFloat("shadowBias", &environment.settings.shadowBias, 0.1f, 0, 1, "%.6f");
    //ImGui::Checkbox("shadowBackFaceRender", &environment.settings.shadowBackFaceRender);

    //ImGui::Spacing();ImGui::Spacing();

    //ImGui::DrawEnumCombo<AntiAliasing>("antiAliasing", &environment.settings.antiAliasing);

    /*if(environment.settings.antiAliasing == AntiAliasing::MSAA){
        ImGui::DrawEnumCombo<MSAAQuality>("msaaQuality", &environment.settings.msaaQuality);
    }*/

    ImGui::Spacing();ImGui::Spacing();

    //ImGui::DrawEnumCombo<ColorCorrection>("colorCorrection", &environment.settings.colorCorrection);

    if(environment.settings.toneMappingPostFX != nullptr){
        if(ImGui::TreeNode("ToneMappingPostFX")){
            environment.settings.toneMappingPostFX->OnGui();
            ImGui::TreePop();
        }
    }

    if(environment.settings.colorGradingPostFX != nullptr){
        if(ImGui::TreeNode("ColorGradingPostFX")){
            environment.settings.colorGradingPostFX->OnGui();
            ImGui::TreePop();
        }
    }

    if(environment.settings.bloomPostFX != nullptr){
        if(ImGui::TreeNode("BloomPostFX")){
            environment.settings.bloomPostFX->OnGui();
            ImGui::TreePop();
        }
    }
}

}