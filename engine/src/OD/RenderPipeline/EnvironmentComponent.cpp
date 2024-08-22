#include "EnvironmentComponent.h"
#include "OD/Serialization/CerealImGui.h"

namespace OD{

void EnvironmentComponent::OnGui(Entity& e){
    EnvironmentComponent& environment = e.GetComponent<EnvironmentComponent>();

    ImGui::DrawEnumCombo<EnvironmentSky>("environmentSky", &environment.settings.environmentSky);
    if(environment.settings.environmentSky == EnvironmentSky::Cubemap){
        std::string skyCubemap("skyCubemap");;
        ImGui::DrawAsset<Cubemap>(skyCubemap, environment.settings.skyCubemap);
    }
    if(environment.settings.environmentSky == EnvironmentSky::Color){
        ImGui::ColorEdit3("skyColor", &environment.settings.cleanColor);
    }
    if(environment.settings.environmentSky == EnvironmentSky::CustomMaterial){
        std::string skyCustomMaterial("skyCustomMaterial");
        ImGui::DrawAsset<Material>(skyCustomMaterial, environment.settings.skyCustomMaterial);
    }

    ImGui::Spacing();ImGui::Spacing();

    ImGui::DrawEnumCombo<EnvironmentLight>("environmentLight", &environment.settings.environmentLight);
    if(environment.settings.environmentLight == EnvironmentLight::Color){
        ImGui::ColorEdit3("ambient", &environment.settings.ambient, ImGuiColorEditFlags_HDR);
    } else {
        ImGui::DragFloat("skyLightIntensity", &environment.settings.skyLightIntensity, 1, 0, 10);
    }

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