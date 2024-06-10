#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/Color.h"
//#include "OD/Graphics/Font.h"
//#include "OD/Graphics/Material.h"

namespace OD{

struct Entity;
class Font;
class Material;

struct OD_API TextRendererComponent{
    std::string text;
    float scale = 1;
    Color color;
    bool is3d = false;

    Ref<Font> font = nullptr;
    Ref<Material> material = nullptr;

    static void OnGui(Entity& e);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(text));
        ArchiveDump(ar, CEREAL_NVP(scale));
        ArchiveDump(ar, CEREAL_NVP(color));
        ArchiveDump(ar, CEREAL_NVP(is3d));

        AssetRef<Font> fontRef(font);
        ArchiveDump(ar, CEREAL_NVP(fontRef));
        AssetRef<Material> materialRef(material);
        ArchiveDump(ar, CEREAL_NVP(materialRef));
    }
};

}