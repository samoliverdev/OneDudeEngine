#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

struct PassRenderSettings{
    bool drawShadow = true;
    bool drawPostProcessing = true;
    bool drawUI = true;
    bool drawGizmos = true;
    
    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, drawShadow);
        ArchiveDumpNVP(ar, drawPostProcessing);
        ArchiveDumpNVP(ar, drawUI);
        ArchiveDumpNVP(ar, drawGizmos);
    }
};

struct PassCollectSettings{
    bool collectStatic = true;
    bool collectDynamic = true;

    bool collectModel = true;
    bool collectMesh = true;
    bool collectSkinnedModel = true;
    bool collectSkinnedMesh = true;
    bool collectCluster = true;

    bool collectDecal = true;
    bool collectParticle = true;
    
    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, collectStatic);
        ArchiveDumpNVP(ar, collectDynamic);

        ArchiveDumpNVP(ar, collectModel);
        ArchiveDumpNVP(ar, collectMesh);

        ArchiveDumpNVP(ar, collectSkinnedModel);
        ArchiveDumpNVP(ar, collectSkinnedMesh);
        ArchiveDumpNVP(ar, collectCluster);

        ArchiveDumpNVP(ar, collectDecal);
        ArchiveDumpNVP(ar, collectParticle);
    }

    uint32_t rejectIfAny = 0;

    void BuildMask();
};

}