#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/SerializationCore.h"
#include <cstdint>
#include <initializer_list>
#include <string>

namespace OD{

enum OD_API_IMPORT Layers: uint32_t{
    Layer0 = 0,
    Layer1,
    Layer2,
    Layer3, 
    Layer4,
    Layer5, 
    Layer6, 
    Layer7, 
    Layer8,
    Layer9,
    Layer10,
    Layer11,
    Layer12,
    Layer13,
    Layer14,
    Layer15,
    Layer16,
    Layer17,
    Layer18,
    Layer19,
    Layer20,
    Layer21,
    Layer22,
    Layer23,
    Layer24,
    Layer25,
    Layer26,
    Layer27,
    Layer28,
    Layer29,
    Layer30,
    LayerCount // always last
};

constexpr int LayerNone = -1;
/*constexpr int LayerMax = 10;
constexpr int AllLayers = Layer0 | Layer1 | Layer2 | Layer3 | Layer4 | Layer5 | Layer6 | Layer7 | Layer8 | Layer9;*/

constexpr uint32_t AllLayersMask = (1u << LayerCount) - 1;
constexpr uint32_t NoneLayersMask = 0;

inline constexpr uint32_t LayerToMask(int layerIndex){
    return (1u << layerIndex);
}

struct OD_API LayerMask{
    //int mask = AllLayers;
    
    static int GetLayerByName(const std::string& name);

    uint32_t mask = AllLayersMask;

    static inline LayerMask FromLayer(int layer) { return { LayerToMask(layer) }; }
    static inline LayerMask FromLayers(std::initializer_list<int> layers) {
        uint32_t m = 0;
        for (int l : layers) m |= LayerToMask(l);
        return { m };
    }

    bool Contains(int layer) const { return (mask & LayerToMask(layer)) != 0; }

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, mask);
    }
};

}
