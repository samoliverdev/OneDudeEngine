#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Material.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include "ToneMappingPostFX.h"
#include "ColorGradingPostFX.h"
#include "BloomPostFX.h"

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

struct OD_API EnvironmentSettings{
    Vector3 ambient = {0.1f, 0.1f, 0.1f};
    Vector3 cleanColor = {0.5f, 0.1f, 0.8f};
    Ref<Material> sky = nullptr;
    Ref<Cubemap> skyboxCubemap;

    ShadowQuality shadowQuality = ShadowQuality::Ultra;
    float shadowBias = 0.00001f;
    bool shadowBackFaceRender = true;

    AntiAliasing antiAliasing;
    MSAAQuality msaaQuality = MSAAQuality::MSAA_4;

    ColorCorrection colorCorrection = ColorCorrection::None;

    Ref<ToneMappingPostFX> toneMappingPostFX = nullptr;
    Ref<ColorGradingPostFX> colorGradingPostFX = nullptr;
    Ref<BloomPostFX> bloomPostFX = nullptr;
    
    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(ambient));
        ArchiveDump(ar, CEREAL_NVP(cleanColor));
        ArchiveDump(ar, CEREAL_NVP(shadowQuality));
        ArchiveDump(ar, CEREAL_NVP(shadowBias));
        ArchiveDump(ar, CEREAL_NVP(shadowBackFaceRender));
        ArchiveDump(ar, CEREAL_NVP(antiAliasing));
        ArchiveDump(ar, CEREAL_NVP(msaaQuality));
        ArchiveDump(ar, CEREAL_NVP(colorCorrection));
        ArchiveDump(ar, CEREAL_NVP(toneMappingPostFX));
        ArchiveDump(ar, CEREAL_NVP(colorGradingPostFX));
        ArchiveDump(ar, CEREAL_NVP(bloomPostFX));
    }
};

struct OD_API EnvironmentComponent{
    //OD_REGISTER_CORE_COMPONENT_TYPE(EnvironmentComponent)
    friend class StandRenderPipeline;

    EnvironmentSettings settings;

    static void OnGui(Entity& e);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(settings));
    }

private:
    bool inited = false;

    inline void Init(){
        inited = true;

        settings.skyboxCubemap = Cubemap::CreateFromFile(
            "res/Engine/Textures/Skybox/right.jpg",
            "res/Engine/Textures/Skybox/left.jpg",
            "res/Engine/Textures/Skybox/top.jpg",
            "res/Engine/Textures/Skybox/bottom.jpg",
            "res/Engine/Textures/Skybox/front.jpg",
            "res/Engine/Textures/Skybox/back.jpg"
        );
        Assert(settings.skyboxCubemap != nullptr);

        settings.sky = CreateRef<Material>();
        settings.sky->SetShader(AssetManager::Get().LoadAsset<Shader>("res/Engine/Shaders/SkyboxCubemap.glsl"));
        //settings.sky->SetShader(AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/SkyboxGradient.glsl"));
        settings.sky->SetCubemap("mainTex", settings.skyboxCubemap);

        settings.toneMappingPostFX = CreateRef<ToneMappingPostFX>();
        settings.toneMappingPostFX->enable = false;

        settings.colorGradingPostFX = CreateRef<ColorGradingPostFX>();
        settings.colorGradingPostFX->enable = false;

        settings.bloomPostFX = CreateRef<BloomPostFX>();
        settings.bloomPostFX->enable = false;
    }
};


}