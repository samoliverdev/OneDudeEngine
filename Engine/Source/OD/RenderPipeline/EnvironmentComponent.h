#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/Serialization.h"
#include "ToneMappingPostFX.h"
#include "ColorGradingPostFX.h"
#include "BloomPostFX.h"
#include "SSAOPostFX.h"
#include "SSGIPostFX.h"

namespace OD{

class Framebuffer;
class Material;
class Cubemap;

enum class ShadowQuality{ VeryLow = 0, Low, Median, High, VeryHigh, Ultra };
//enum class AntiAliasing{ None, MSAA };
//enum class MSAAQuality{ MSAA_2, MSAA_4, MSAA_8 };

enum class EnvironmentSky{
    Cubemap, Color, CustomMaterial, None
};

enum class EnvironmentLight{
    SkyCubemap, Color
};

struct OD_API EnvironmentSettings{
    EnvironmentSky environmentSky = EnvironmentSky::Cubemap;
    EnvironmentLight environmentLight = EnvironmentLight::Color;

    Color ambient = {0.1f, 0.1f, 0.1f, 1};
    Color cleanColor = {0.5f, 0.1f, 0.8f, 1};
    Ref<Material> skyCustomMaterial = nullptr;
    Ref<Cubemap> skyCubemap = nullptr;
    Ref<Cubemap> skyIrradianceMap = nullptr;
    Ref<Cubemap> skyPrefilterMap = nullptr;

    Ref<Framebuffer> skyIrradianceMapF = nullptr;
    Ref<Framebuffer> skyPrefilterMapF = nullptr;

    float skyLightIntensity = 1;

    ShadowQuality directionalshadowQuality = ShadowQuality::High;
    ShadowQuality othershadowQuality = ShadowQuality::Median;
    std::array<float, 4> directinalShadowCascade = {0.1f, 0.25f, 0.5f, 1.0f};

    float shadowDistance = 150;
    float shadowBias = 0.00001f;
    //bool shadowBackFaceRender = true;
    //AntiAliasing antiAliasing;
    //MSAAQuality msaaQuality = MSAAQuality::MSAA_4;

    bool enableSSS;
    float sssSurfaceThickness = 0.005f;
    float sssBilinearThreshold = 0.02f;
    float sssShadowContrast = 4;

    Ref<ToneMappingPostFX> toneMappingPostFX = CreateRef<ToneMappingPostFX>();
    Ref<ColorGradingPostFX> colorGradingPostFX = CreateRef<ColorGradingPostFX>();;
    Ref<BloomPostFX> bloomPostFX = CreateRef<BloomPostFX>();
    Ref<SSAOPostFX> ssaoPostFX = CreateRef<SSAOPostFX>();
    Ref<SSGIPostFX> ssgiPostFX = CreateRef<SSGIPostFX>();

    std::vector<Ref<PostFX>> customPostPrecessings;
    
    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, environmentSky);
        ArchiveDumpNVP(ar, environmentLight);
        ArchiveDumpNVP(ar, ambient);
        ArchiveDumpNVP(ar, cleanColor);
        ArchiveDumpNVP(ar, directionalshadowQuality);
        ArchiveDumpNVP(ar, othershadowQuality);
        ArchiveDumpNVP(ar, directinalShadowCascade);
        ArchiveDumpNVP(ar, shadowDistance);
        ArchiveDumpNVP(ar, shadowBias);

        ArchiveDumpNVP(ar, enableSSS);
        ArchiveDumpNVP(ar, sssSurfaceThickness);
        ArchiveDumpNVP(ar, sssBilinearThreshold);
        ArchiveDumpNVP(ar, sssShadowContrast);

        ArchiveDumpNVP(ar, toneMappingPostFX);
        ArchiveDumpNVP(ar, colorGradingPostFX);
        ArchiveDumpNVP(ar, bloomPostFX);
        ArchiveDumpNVP(ar, ssaoPostFX);
        ArchiveDumpNVP(ar, ssgiPostFX);

        AssetRefSerialize<Cubemap> skyCubemap_(skyCubemap);
        ArchiveDumpNamed(ar, "skyCubemap", skyCubemap_);
    }

    //bool hasInited = false;

    EnvironmentSettings();
};

struct OD_API EnvironmentComponent{
    friend class StandRenderPipeline;

    EnvironmentSettings settings;

    static void OnGui(Entity& e, Scene& scene);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, settings);
    }

    EnvironmentComponent(){
        /*settings.skyCubemap = Cubemap::CreateFromFile(
            "Engine/Textures/Skybox/right.jpg",
            "Engine/Textures/Skybox/left.jpg",
            "Engine/Textures/Skybox/top.jpg",
            "Engine/Textures/Skybox/bottom.jpg",
            "Engine/Textures/Skybox/front.jpg",
            "Engine/Textures/Skybox/back.jpg"
        );
        
        settings.skyIrradianceMap = Cubemap::CreateIrradianceMapFromCubeMap(settings.skyCubemap);
        settings.skyPrefilterMap = Cubemap::CreatePrefilterMapFromCubeMap(settings.skyCubemap);

        Assert(settings.skyCubemap != nullptr);

        settings.skyCustomMaterial = CreateRef<Material>();
        settings.skyCustomMaterial->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/SkyboxCubemap.glsl"));
        //settings.sky->SetShader(AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/SkyboxGradient.glsl"));
        settings.skyCustomMaterial->SetCubemap("mainTex", settings.skyCubemap);*/
    }
};


}