#include "EnvironmentComponent.h"
#include "OD/Utils/ImGuiCustomDraw.h"
#include "OD/Serialization/CerealImGui.h"

namespace OD{

int ShadowQualityLookup[] = {512, 1024, 2048, 4096, 8192};
const char* ShadowQualityLookupNames[] = {"Low", "High", "VeryHigh", "Ultra", "VeryUltra"};

const char* AntiAliasingLookupNames[] = {"None", "MSAA"};

int MSAAQualityLookup[] = {2, 4, 8};
const char* MSAAQualityLookupNames[] = {"MSAA_2", "MSAA_4", "MSAA_8"};

const char* ColorCorrectionLookupNames[] = {"None", "ColorCorrection"};

void EnvironmentComponent::OnGui(Entity& e){
    EnvironmentComponent& environment = e.GetComponent<EnvironmentComponent>();

    float ambient[] = {
        environment.settings.ambient.x, 
        environment.settings.ambient.y, 
        environment.settings.ambient.z
    };
    if(ImGui::ColorEdit3("ambient", ambient)){
        environment.settings.ambient = Vector3(ambient[0], ambient[1], ambient[2]);
    }

    float cleanColor[] = {
        environment.settings.cleanColor.x, 
        environment.settings.cleanColor.y, 
        environment.settings.cleanColor.z
    };
    if(ImGui::ColorEdit3("cleanColor", cleanColor)){
        environment.settings.cleanColor = Vector3(cleanColor[0], cleanColor[1], cleanColor[2]);
    }
    
    std::string sky("sky");
    ImGui::DrawMaterialAsset(sky, environment.settings.sky);

    ImGui::Spacing();ImGui::Spacing();

    ImGui::DrawEnumCombo<ShadowQuality>(
        "shadowQuality", 
        environment.settings.shadowQuality, 
        ShadowQualityLookupNames,
        5
    );

    ImGui::DragFloat("shadowBias", &environment.settings.shadowBias, 0.1f, 0, 1, "%.6f");
    ImGui::Checkbox("shadowBackFaceRender", &environment.settings.shadowBackFaceRender);

    ImGui::Spacing();ImGui::Spacing();

    ImGui::DrawEnumCombo<AntiAliasing>(
        "antiAliasing", 
        environment.settings.antiAliasing, 
        AntiAliasingLookupNames,
        2
    );

    if(environment.settings.antiAliasing == AntiAliasing::MSAA){
        ImGui::DrawEnumCombo<MSAAQuality>(
            "msaaQuality", 
            environment.settings.msaaQuality, 
            MSAAQualityLookupNames,
            3
        );
    }

    ImGui::Spacing();ImGui::Spacing();

    ImGui::DrawEnumCombo<ColorCorrection>(
        "colorCorrection", 
        environment.settings.colorCorrection, 
        ColorCorrectionLookupNames,
        2
    );

    if(environment.settings.toneMappingPostFX != nullptr){
        if(ImGui::TreeNode("ToneMappingPostFX")){
            cereal::ImGuiArchive toneMappingPostFX;
            toneMappingPostFX(*environment.settings.toneMappingPostFX);
            ImGui::TreePop();
        }
    }

    if(environment.settings.colorGradingPostFX != nullptr){
        if(ImGui::TreeNode("ColorGradingPostFX")){
            cereal::ImGuiArchive colorGradring;
            colorGradring(*environment.settings.colorGradingPostFX);
            ImGui::TreePop();
        }
    }

    if(environment.settings.bloomPostFX != nullptr){
        if(ImGui::TreeNode("BloomPostFX")){
            cereal::ImGuiArchive colorGradring;
            colorGradring(*environment.settings.bloomPostFX);
            ImGui::TreePop();
        }
    }
}

}