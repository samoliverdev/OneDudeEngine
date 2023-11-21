#include "Skeleton.h"

namespace OD{

Skeleton::Skeleton(){}

Skeleton::Skeleton(const Pose& rest, const Pose& bind, const std::vector<std::string>& names){
    Set(rest, bind, names);
}

void Skeleton::Set(const Pose& rest, const Pose& bind, const std::vector<std::string>& names){
    _restPose = rest;
    _bindPose = bind;
    _jointNames = names;
    UpdateInverseBindPose();
}

Pose& Skeleton::GetBindPose(){ return _bindPose; }
Pose& Skeleton::GetRestPose(){ return _restPose; }
std::vector<Matrix4>& Skeleton::GetInvBindPose(){ return _invBindPose; }
std::vector<std::string>& Skeleton::GetJointNames(){ return _jointNames; }
std::string& Skeleton::GetJointName(unsigned int index){ return _jointNames[index]; }

void Skeleton::UpdateInverseBindPose(){
    unsigned int size = _bindPose.Size();
    _invBindPose.resize(size);

    for(unsigned int i = 0; i < size; ++i){
        Transform world = _bindPose.GetGlobalTransform(i);
        _invBindPose[i] = math::inverse(world.GetLocalModelMatrix());
    }
}

}