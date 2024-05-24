#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Core/Color.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Graphics/Font.h"

namespace OD{

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
        //ArchiveDump(ar, CEREAL_NVP(font));
        //ArchiveDump(ar, CEREAL_NVP(material));
    }
};

}