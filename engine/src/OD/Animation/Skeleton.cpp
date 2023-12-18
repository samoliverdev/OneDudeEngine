#include "Skeleton.h"

namespace OD{

Skeleton::Skeleton(){}

Skeleton::Skeleton(const Pose& rest, const Pose& bind, const std::vector<std::string>& names){
    Set(rest, bind, names);
}

void Skeleton::Set(const Pose& rest, const Pose& bind, const std::vector<std::string>& names){
    restPose = rest;
    bindPose = bind;
    jointNames = names;
    UpdateInverseBindPose();
}

Pose& Skeleton::GetBindPose(){ return bindPose; }
Pose& Skeleton::GetRestPose(){ return restPose; }
std::vector<Matrix4>& Skeleton::GetInvBindPose(){ return invBindPose; }
std::vector<std::string>& Skeleton::GetJointNames(){ return jointNames; }
std::string& Skeleton::GetJointName(unsigned int index){ return jointNames[index]; }

void Skeleton::UpdateInverseBindPose(){
    unsigned int size = bindPose.Size();
    invBindPose.resize(size);

    for(unsigned int i = 0; i < size; ++i){
        //Transform world = _bindPose.GetGlobalTransform(i);
        //_invBindPose[i] = math::inverse(world.GetLocalModelMatrix());

        invBindPose[i] = math::inverse(bindPose.GetGlobalMatrix(i));
    }
}

}