#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "Pose.h"
#include <vector>
#include <string>

namespace OD{

class OD_API Skeleton{
public:
    Skeleton();
    Skeleton(const Pose& rest, const Pose& bind, const std::vector<std::string>& names);

    void Set(const Pose& rest, const Pose& bind, const std::vector<std::string>& names);

    Pose& GetBindPose();
    Pose& GetRestPose();
    AlignedVector<Matrix4> & GetInvBindPose();
    std::vector<std::string>& GetJointNames();
    std::string& GetJointName(unsigned int index);
    int FindJointByName(const std::string& name);
    inline void Clear(){ restPose.Clear(); bindPose.Clear(); invBindPose.clear(); jointNames.clear(); }

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, restPose);
        ArchiveDump(ar, bindPose);
        ArchiveDump(ar, invBindPose);
        ArchiveDump(ar, jointNames);
    }

protected:
    Pose restPose;
    Pose bindPose;
    AlignedVector<Matrix4> invBindPose;
    std::vector<std::string> jointNames;

    void UpdateInverseBindPose();
};

}