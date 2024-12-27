#pragma once
#include "OD/Defines.h"
#include "OD/Graphics/Texture.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

struct OD_API CanvasComponent{
    enum class ScalerMode{
        ConstatPixelSize,
        ScaleWithScreenSize
    };
    enum class ScreenMatchMode{
        //MatchWidthOrHeight
        MatchWidth,
        MatchHeight,
        Expand
    };

    ScalerMode scalerMode;
    ScreenMatchMode screenMatchMode;

    float scale = 1;
    Vector2 size = {1280, 720};

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(scalerMode));
        ArchiveDump(ar, CEREAL_NVP(screenMatchMode));
        ArchiveDump(ar, CEREAL_NVP(scale));
        ArchiveDump(ar, CEREAL_NVP(size));
    }

    inline Vector2 GetResulutionScale(float width, float height){
        if(scalerMode == ScalerMode::ConstatPixelSize){
            return {scale, scale};
        }
        if(scalerMode == ScalerMode::ScaleWithScreenSize && screenMatchMode == ScreenMatchMode::MatchWidth){
            Vector2 resulutionOffset = {width/size.x, height/size.y};
            Vector2 resulutionScale = {resulutionOffset.x/resulutionOffset.y, resulutionOffset.x/resulutionOffset.y};
            return resulutionScale;
        }
        if(scalerMode == ScalerMode::ScaleWithScreenSize && screenMatchMode == ScreenMatchMode::MatchHeight){
            Vector2 resulutionOffset = {width/size.x, height/size.y};
            Vector2 resulutionScale = {resulutionOffset.y/resulutionOffset.x, resulutionOffset.y/resulutionOffset.x};
            return resulutionScale;
        }
        if(scalerMode == ScalerMode::ScaleWithScreenSize && screenMatchMode == ScreenMatchMode::Expand){
            Vector2 resulutionOffset = {width/size.x, height/size.y};
            Vector2 resulutionScale = {resulutionOffset.x, resulutionOffset.y};
            return resulutionScale;
        }

        return {1, 1};
    }
};

struct OD_API RectTransformComponet{
    Vector2 pos = {0, 0};

    Vector2 anchors = {0, 0};
    Vector2 size = {100, 100};

    Entity canvasRoot;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(pos));
        ArchiveDump(ar, CEREAL_NVP(size));
        ArchiveDump(ar, CEREAL_NVP(anchors));
    }
};

struct OD_API UIImageComponent{
    Ref<Texture2D> sourceImage = nullptr;
    Color color = {1, 1, 1, 1};
    Ref<Material> material = nullptr;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(color));

        AssetRefSerialize<Texture2D> texRef(sourceImage);
        ArchiveDump(ar, CEREAL_NVP(texRef));
        AssetRefSerialize<Material> materialRef(material);
        ArchiveDump(ar, CEREAL_NVP(materialRef));
    }
};

struct OD_API UITextComponent{
    std::string text;
    float scale = 1;
    Color color;

    Ref<Font> font = nullptr; //AssetManager::Get().LoadAsset<Font>("res/Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
    Ref<Material> material = nullptr; //CreateRef<Material>(Shader::CreateFromFile("res/Engine/Shaders/Font.glsl"));

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(text));
        ArchiveDump(ar, CEREAL_NVP(scale));
        ArchiveDump(ar, CEREAL_NVP(color));

        AssetRefSerialize<Font> fontRef(font);
        ArchiveDump(ar, CEREAL_NVP(fontRef));
        AssetRefSerialize<Material> materialRef(material);
        ArchiveDump(ar, CEREAL_NVP(materialRef));
    }
};

}