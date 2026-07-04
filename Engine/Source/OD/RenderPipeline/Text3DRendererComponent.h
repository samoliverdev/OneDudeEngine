#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/Color.h"
#include "OD/Scene/Scene.h"

namespace OD{

class Shader;
class Material;
class Texture2D;
class Font;

struct OD_API Text3DRendererComponent{
    std::string text;
    //float scale = 1;
    Color color;

    Ref<Font> font = nullptr; //AssetManager::Get().LoadAsset<Font>("Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
    Ref<Material> material = nullptr; //CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/FontMSDF.glsl"));

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(text));
        //ArchiveDump(ar, CEREAL_NVP(scale));
        ArchiveDump(ar, CEREAL_NVP(color));

        AssetRefSerialize<Font> fontRef(font);
        ArchiveDump(ar, CEREAL_NVP(fontRef));
        AssetRefSerialize<Material> materialRef(material);
        ArchiveDump(ar, CEREAL_NVP(materialRef));
    }
};

}