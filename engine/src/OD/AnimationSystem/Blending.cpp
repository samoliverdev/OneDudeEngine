#include "Blending.h"

namespace OD{

bool IsInHierarchy(Pose& pose, unsigned int parent, unsigned int search){
    if(search == parent) return true;

    int p = pose.GetParent(search);

    while(p >= 0){
        if(p == (int)parent){
            return true;
        }
        p = pose.GetParent(p);
    }

    return false;
}

void Blend(Pose& output, Pose& a, Pose& b, float t, int root){
    unsigned int numJoints = output.Size();
    for(unsigned int i = 0; i < numJoints; i++){
        if(root >= 0){
            if(!IsInHierarchy(output, root, i)){
                continue;
            }
        }

        output.SetLocalTransform(i, Transform::mix(a.GetLocalTransform(i), b.GetLocalTransform(i), t));
    }
}

Pose MakeAdditivePose(Skeleton& skeleton, Clip& clip){
	Pose result = skeleton.GetRestPose();
	clip.Sample(result, clip.GetStartTime());
	return result;
}

void Add(Pose& output, Pose& inPose, Pose& addPose, Pose& basePose, int blendroot){
	unsigned int numJoints = addPose.Size();
	for(unsigned int i = 0; i < numJoints; ++i){
		Transform input = inPose.GetLocalTransform(i);
		Transform additive = addPose.GetLocalTransform(i);
		Transform additiveBase = basePose.GetLocalTransform(i);

		if(blendroot >= 0 && !IsInHierarchy(addPose, blendroot, i)){
			continue;
		}

		// outPose = inPose + (addPose - basePose)
		Transform result(
			input.localPosition() + (additive.localPosition() - additiveBase.localPosition()),
			math::normalize(input.localRotation() * (math::inverse(additiveBase.localRotation()) * additive.localRotation())),
			input.localScale() + (additive.localScale() - additiveBase.localScale())
		);
		output.SetLocalTransform(i, result);
	}
}

}