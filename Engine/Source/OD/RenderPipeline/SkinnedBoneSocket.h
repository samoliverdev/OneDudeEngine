#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

struct OD_API SkinnedBoneSocket{
    int boneIndex = -1;

    Vector3 offset;
    Vector3 OffsetEuler;

    bool useRootLocalOffsets = false;
    Vector3 rootLocalPos;
    Quaternion rootLocalRot;

    template <class Archive> 
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, boneIndex);
        ArchiveDumpNVP(ar, offset);
        ArchiveDumpNVP(ar, OffsetEuler);

        ArchiveDumpNVP(ar, useRootLocalOffsets);
        ArchiveDumpNVP(ar, rootLocalPos);
        ArchiveDumpNVP(ar, rootLocalRot);
    }
};

}