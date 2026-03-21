#pragma once
#include "OD/Defines.h"
#include "OD/Core/Color.h"
#include "OD/Graphics/Texture.h"
#include "OD/Graphics/Material.h"
#include "OD/Serialization/SerializationCore.h"
#include "OD/Serialization/SerializationMath.h"

namespace OD{

struct OD_API SpriteRendererComponent{
    Ref<Texture2D> sprite = nullptr;
    Color color;
    bool flipX;
    bool flipY;
    Ref<Material> material = nullptr;
    
    float pixelUnitSize = 100;

    //static void OnGui(Entity& e);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(color));
        ArchiveDump(ar, CEREAL_NVP(flipX));
        ArchiveDump(ar, CEREAL_NVP(flipY));
        ArchiveDump(ar, CEREAL_NVP(pixelUnitSize));

        AssetRefSerialize<Texture2D> texRef(sprite);
        ArchiveDump(ar, CEREAL_NVP(texRef));
        AssetRefSerialize<Material> materialRef(material);
        ArchiveDump(ar, CEREAL_NVP(materialRef));
    }
};

};