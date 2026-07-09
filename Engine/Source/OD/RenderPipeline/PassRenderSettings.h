#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

struct PassRenderSettings{
    bool drawShadow;
    bool drawPostProcessing;
    bool drawUI;
    bool drawGizmos;
    
    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, drawShadow);
        ArchiveDumpNVP(ar, drawPostProcessing);
        ArchiveDumpNVP(ar, drawUI);
        ArchiveDumpNVP(ar, drawGizmos);
    }
};

}