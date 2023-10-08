#include "EnvironmentComponent.h"
#include "OD/Utils/ImGuiCustomDraw.h"

namespace OD{

int ShadowQualityLookup[] = {512, 1024, 2048, 4096, 8192};
const char* ShadowQualityLookupNames[] = {"Low", "High", "VeryHigh", "Ultra", "VeryUltra"};

const char* AntiAliasingLookupNames[] = {"None", "MSAA"};

int MSAAQualityLookup[] = {2, 4, 8};
const char* MSAAQualityLookupNames[] = {"MSAA_2", "MSAA_4", "MSAA_8"};

const char* ColorCorrectionLookupNames[] = {"None", "ColorCorrection"};

void EnvironmentComponent::Serialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "EnvironmentComponent";
    out << YAML::BeginMap;

    auto& c = e.GetComponent<EnvironmentComponent>();
    out << YAML::Key << "settings.ambient" << YAML::Value << c.settings.ambient;
    out << YAML::Key << "settings.cleanColor" << YAML::Value << c.settings.cleanColor;
    
    if(c.settings.sky != nullptr && c.settings.sky->path() != "Memory"){
        out << YAML::Key << "settings.sky" << YAML::Value << c.settings.sky->path();
    } else {
        out << YAML::Key << "settings.sky" << YAML::Value << std::string();
    }

    out << YAML::Key << "settings.shadowQuality" << YAML::Value << (int)c.settings.shadowQuality;
    out << YAML::Key << "settings.shadowBias" << YAML::Value << c.settings.shadowBias;
    out << YAML::Key << "settings.shadowBackFaceRender" << YAML::Value << c.settings.shadowBackFaceRender;

    out << YAML::Key << "settings.antiAliasing" << YAML::Value << (int)c.settings.antiAliasing;
    out << YAML::Key << "settings.msaaQuality" << YAML::Value << (int)c.settings.msaaQuality;

    out << YAML::Key << "settings.colorCorrection" << YAML::Value << (int)c.settings.colorCorrection;

    out << YAML::EndMap;
}

void EnvironmentComponent::Deserialize(YAML::Node& in, Entity& e){
    auto& c = e.AddOrGetComponent<EnvironmentComponent>();

    c.settings.ambient = in["settings.ambient"].as<Vector3>();
    c.settings.cleanColor = in["settings.cleanColor"].as<Vector3>();

    std::string sky = in["settings.sky"].as<std::string>();
    if(sky.empty()){
        c.settings.sky = nullptr;
    } else {
        c.settings.sky = AssetManager::Get().LoadMaterial(sky);
    }
    
    c.settings.shadowQuality = (ShadowQuality)in["settings.shadowQuality"].as<int>();
    c.settings.shadowBias = in["settings.shadowBias"].as<float>();
    c.settings.shadowBackFaceRender = in["settings.shadowBackFaceRender"].as<bool>();

    c.settings.antiAliasing = (AntiAliasing)in["settings.antiAliasing"].as<int>();
    c.settings.msaaQuality = (MSAAQuality)in["settings.msaaQuality"].as<int>();

    c.settings.colorCorrection = (ColorCorrection)in["settings.colorCorrection"].as<int>();
}

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
    
    ImGui::DrawMaterialAsset("sky", environment.settings.sky);

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
}

}