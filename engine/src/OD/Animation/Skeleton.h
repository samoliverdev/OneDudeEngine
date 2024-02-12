#pragma once

#include <vector>
#include <string>
#include "Pose.h"

namespace OD{

class Skeleton{
public:
    Skeleton();
    Skeleton(const Pose& rest, const Pose& bind, const std::vector<std::string>& names);

    void Set(const Pose& rest, const Pose& bind, const std::vector<std::string>& names);

    Pose& GetBindPose();
    Pose& GetRestPose();
    std::vector<Matrix4>& GetInvBindPose();
    std::vector<std::string>& GetJointNames();
    std::string& GetJointName(unsigned int index);

protected:
    Pose restPose;
    Pose bindPose;
    std::vector<Matrix4> invBindPose;
    std::vector<std::string> jointNames;

    void UpdateInverseBindPose();
};

}