#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/SerializationCore.h"
#include "OD/Serialization/SerializationMath.h"
#include "OD/Core/Color.h"
#include "OD/Scene/Scene.h"

namespace OD{

class Material;
class Texture2D;
class Font;
struct Camera; 

struct OD_API CanvasComponent{
    enum class ScaleMode {
        ConstantPixelSize,
        ScaleWithScreenSize
    };
    enum class ScreenMatchMode {
        MatchWidthOrHeight,
        MatchWidth,
        MatchHeight,
        Expand
    };

    Vector2 referenceResolution = {1920, 1080};
    ScaleMode scaleMode = ScaleMode::ScaleWithScreenSize;
    ScreenMatchMode screenMatchMode = ScreenMatchMode::MatchWidthOrHeight;
    float matchValue = 0.5f;
    float defaultScale = 1;
    float scaleFactor = 1.0f;

    void CalculateCanvasScale(float width, float height);

    template <class Archive>
    void serialize(Archive& ar) {
        ArchiveDump(ar, CEREAL_NVP(referenceResolution));
        ArchiveDump(ar, CEREAL_NVP(scaleMode));
        ArchiveDump(ar, CEREAL_NVP(screenMatchMode));
        ArchiveDump(ar, CEREAL_NVP(matchValue));
        ArchiveDump(ar, CEREAL_NVP(defaultScale));
    }
};

void OD_API RecalculateUI(Scene& scene, Camera& camera);

struct OD_API RectTransformComponent{
    Vector2 anchorMin = {0.0f, 0.0f};
    Vector2 anchorMax = {1.0f, 1.0f};
    Vector2 pivot = {0.5f, 0.5f};
    Vector2 offsetMin = {0.0f, 0.0f};
    Vector2 offsetMax = {0.0f, 0.0f};
    Vector2 finalPosition;
    Vector2 finalSize;

    inline void SetRect(Vector2 anchor, Vector2 pivot, Vector2 position, Vector2 size) {
        anchorMin = anchor;
        anchorMax = anchor;
        this->pivot = pivot;
        offsetMin = position - size * pivot;
        offsetMax = offsetMin + size;
    }

    inline void SetRectStretch(Vector2 anchorMin_, Vector2 anchorMax_, Vector2 offsetMin_, Vector2 offsetMax_, Vector2 pivot_ = {0.5f, 0.5f}) {
        anchorMin = anchorMin_;
        anchorMax = anchorMax_;
        offsetMin = offsetMin_;
        offsetMax = offsetMax_;
        pivot = pivot_;
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ArchiveDumpNVP(ar, anchorMin);
        ArchiveDumpNVP(ar, anchorMax);
        ArchiveDumpNVP(ar, pivot);
        ArchiveDumpNVP(ar, offsetMin);
        ArchiveDumpNVP(ar, offsetMax);
    }

    static void OnGui(Entity& e, Scene& scene);
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