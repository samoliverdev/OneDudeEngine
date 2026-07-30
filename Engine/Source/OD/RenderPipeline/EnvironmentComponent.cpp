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
    skyCubemap = ResourceManager::Get().LoadByPath<Cubemap>("DefaultSkyboxCubemap");
    
    //skyIrradianceMap = Cubemap::CreateIrradianceMapFromCubeMap(skyCubemap);
    //skyPrefilterMap = Cubemap::CreatePrefilterMapFromCubeMap(skyCubemap);

    Assert(skyCubemap != nullptr);

    skyCustomMaterial = ResourceManager::Get().Create<Material>();
    skyCustomMaterial->SetShader(ResourceManager::Get().LoadByPath<Shader>("Engine/Shaders/SkyboxCubemap.glsl"));
    //settings.sky->SetShader(AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/SkyboxGradient.glsl"));
    skyCustomMaterial->SetCubemap("mainTex", skyCubemap);
}

void EnvironmentComponent::OnGui(Entity& e, Scene& scene){
    EnvironmentComponent& environment = scene.GetComponent<EnvironmentComponent>(e);

    ImGui::DrawEnumCombo<EnvironmentSky>("environmentSky", &environment.settings.environmentSky);
    if(environment.settings.environmentSky == EnvironmentSky::Cubemap){
        std::string skyCubemap("skyCubemap");;
        if(ImGui::DrawAsset<Cubemap>(skyCubemap, environment.settings.skyCubemap)){
            environment.settings.skyIrradianceMap = nullptr;
            environment.settings.skyPrefilterMap = nullptr;
        }
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

    if(environment.settings.toneMapping != nullptr){
        if(ImGui::TreeNode("ToneMapping")){
            environment.settings.toneMapping->OnGui();
            ImGui::TreePop();
        }
    }

    if(environment.settings.colorGrading != nullptr){
        if(ImGui::TreeNode("ColorGrading")){
            environment.settings.colorGrading->OnGui();
            ImGui::TreePop();
        }
    }

    if(environment.settings.bloom != nullptr){
        if(ImGui::TreeNode("Bloom")){
            environment.settings.bloom->OnGui();
            ImGui::TreePop();
        }
    }

    if(environment.settings.ssao != nullptr){
        if(ImGui::TreeNode("SSAO")){
            environment.settings.ssao->OnGui();
            ImGui::TreePop();
        }
    }

    if(environment.settings.ssgi != nullptr){
        if(ImGui::TreeNode("SSGI")){
            environment.settings.ssgi->OnGui();
            ImGui::TreePop();
        }
    }

    /*if(ImGui::TreeNode("CustomPostFX")){
        for(auto& i: environment.settings.customPostPrecessings){
            i->OnGui();
        }
        ImGui::TreePop();
    }*/

    //////////////////////////////////
    const ImGuiTreeNodeFlags treeNodeFlags = 
        ImGuiTreeNodeFlags_DefaultOpen 
        | ImGuiTreeNodeFlags_Framed 
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding;

    
    std::hash<std::string> hasher;
    //ScriptComponent& script = scene.GetComponent<ScriptComponent>(e);

    bool removeScript = false;

    for(auto i: RendererFeatureGlobal::Get().GetNewRendererFeatureFuncs()){
        if(i.second.has(environment.features) == false) continue;

        bool open = ImGui::TreeNodeEx((void*)hasher(i.first.c_str()), treeNodeFlags, i.first.c_str());

        if(ImGui::BeginPopupContextItem()){
            if(ImGui::MenuItem("Remove Feature")){
                removeScript = true;
            }
            ImGui::EndPopup();
        }

        if(open){
            i.second.onGui(environment.features);
            ImGui::TreePop();
        }

        if(removeScript){
            i.second.remove(environment.features);
            break;
        }
    }

    if(ImGui::Button("Add Feature")){
        ImGui::OpenPopup("AddFeature");
    }

    if(ImGui::BeginPopup("AddFeature")){
        for(auto& i: RendererFeatureGlobal::Get().GetNewRendererFeatureFuncs()){
            if(ImGui::MenuItem(i.first.c_str())){
                i.second.add(environment.features);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

}