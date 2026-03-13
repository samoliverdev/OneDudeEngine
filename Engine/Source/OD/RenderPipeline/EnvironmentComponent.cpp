#include "OD/pch.h"
#include "EnvironmentComponent.h"
#include "OD/Serialization/CerealImGui.h"
#include "OD/Graphics/Cubemap.h"
#include "OD/Graphics/Material.h"
#include "OD/Core/ImGui.h"

namespace OD{

EnvironmentSettings::EnvironmentSettings(){
    //return;
    /*skyCubemap = Cubemap::CreateFromFile(
        "Engine/Textures/Skybox/right.jpg",
        "Engine/Textures/Skybox/left.jpg",
        "Engine/Textures/Skybox/top.jpg",
        "Engine/Textures/Skybox/bottom.jpg",
        "Engine/Textures/Skybox/front.jpg",
        "Engine/Textures/Skybox/back.jpg"
    );*/
    skyCubemap = AssetManager::Get().LoadAsset<Cubemap>("DefaultSkyboxCubemap");
    
    //skyIrradianceMap = Cubemap::CreateIrradianceMapFromCubeMap(skyCubemap);
    //skyPrefilterMap = Cubemap::CreatePrefilterMapFromCubeMap(skyCubemap);

    Assert(skyCubemap != nullptr);

    skyCustomMaterial = CreateRef<Material>();
    skyCustomMaterial->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/SkyboxCubemap.glsl"));
    //settings.sky->SetShader(AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/SkyboxGradient.glsl"));
    skyCustomMaterial->SetCubemap("mainTex", skyCubemap);
}

void EnvironmentComponent::OnGui(Entity& e, Scene& scene){
    EnvironmentComponent& environment = scene.GetComponent<EnvironmentComponent>(e);

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
    ImGui::DragFloat4("directinalShadowCascade", &environment.settings.directinalShadowCascade[0]);

    ImGui::DragFloat("shadowDistance", &environment.settings.shadowDistance);
    ImGui::DragFloat("shadowBias", &environment.settings.shadowBias, 0.1f, 0, 1, "%.6f");

    ImGui::Checkbox("enableSSS", &environment.settings.enableSSS);
    ImGui::DragFloat("sssSurfaceThickness", &environment.settings.sssSurfaceThickness);
    ImGui::DragFloat("sssBilinearThreshold", &environment.settings.sssBilinearThreshold);
    ImGui::DragFloat("sssShadowContrast", &environment.settings.sssShadowContrast);

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

    if(environment.settings.ssaoPostFX != nullptr){
        if(ImGui::TreeNode("SSAOPostFX")){
            environment.settings.ssaoPostFX->OnGui();
            ImGui::TreePop();
        }
    }

    if(environment.settings.ssgiPostFX != nullptr){
        if(ImGui::TreeNode("SSGIPostFX")){
            environment.settings.ssgiPostFX->OnGui();
            ImGui::TreePop();
        }
    }

    if(ImGui::TreeNode("CustomPostFX")){
        for(auto& i: environment.settings.customPostPrecessings){
            i->OnGui();
        }
        ImGui::TreePop();
    }
}

}