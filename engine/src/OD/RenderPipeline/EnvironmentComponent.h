#pragma once

#include "OD/Scene/Scene.h"
#include "OD/Graphics/Material.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

namespace OD{

enum class ShadowQuality{Low, High, VeryHigh, Ultra, VeryUltra };
extern int ShadowQualityLookup[];
extern const char* ShadowQualityLookupNames[];

enum class AntiAliasing{ None, MSAA };
extern const char* AntiAliasingLookupNames[];

enum class MSAAQuality{ MSAA_2, MSAA_4, MSAA_8 };
extern int MSAAQualityLookup[];
extern const char* MSAAQualityLookupNames[];

enum class ColorCorrection{ None, ColorCorrection };
extern const char* ColorCorrectionLookupNames[];

struct EnvironmentSettings{
    Vector3 ambient = {0.1f, 0.1f, 0.1f};
    Vector3 cleanColor = {0.5f, 0.1f, 0.8f};
    Ref<Material> sky = nullptr;

    ShadowQuality shadowQuality = ShadowQuality::Ultra;
    float shadowBias = 0.00001f;
    bool shadowBackFaceRender = true;

    AntiAliasing antiAliasing;
    MSAAQuality msaaQuality = MSAAQuality::MSAA_4;

    ColorCorrection colorCorrection = ColorCorrection::None;

    template <class Archive>
    void serialize(Archive & ar){
        ar(
            CEREAL_NVP(ambient),
            CEREAL_NVP(cleanColor),
            CEREAL_NVP(shadowQuality),
            CEREAL_NVP(shadowBias),
            CEREAL_NVP(shadowBackFaceRender),
            CEREAL_NVP(antiAliasing),
            CEREAL_NVP(msaaQuality),
            CEREAL_NVP(colorCorrection)
        );
    }
};

struct EnvironmentComponent{
    //OD_REGISTER_CORE_COMPONENT_TYPE(EnvironmentComponent)
    friend class StandRenderPipeline;
    friend class StandRenderPipeline2;

    EnvironmentSettings settings;

    static void OnGui(Entity& e);

    template <class Archive>
    void serialize(Archive & ar){
        ar(CEREAL_NVP(settings));
    }

private:
    bool inited = false;

    inline void Init(){
        inited = true;

        Ref<Cubemap> skyboxCubemap = Cubemap::CreateFromFile(
            "res/Engine/Textures/Skybox/right.jpg",
            "res/Engine/Textures/Skybox/left.jpg",
            "res/Engine/Textures/Skybox/top.jpg",
            "res/Engine/Textures/Skybox/bottom.jpg",
            "res/Engine/Textures/Skybox/front.jpg",
            "res/Engine/Textures/Skybox/back.jpg"
        );
        Assert(skyboxCubemap != nullptr);

        settings.sky = CreateRef<Material>();
        settings.sky->SetShader(AssetManager::Get().LoadShaderFromFile("res/Engine/Shaders/SkyboxCubemap.glsl"));
        //settings.sky->SetShader(AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/SkyboxGradient.glsl"));
        settings.sky->SetCubemap("mainTex", skyboxCubemap);
    }
};


}