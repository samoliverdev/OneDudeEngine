#include "Pose.h"

namespace OD{

Pose::Pose(){}

Pose::Pose(unsigned int numJoints){
    Resize(numJoints);
}

Pose::Pose(const Pose& p){
    *this = p;
}

Pose& Pose::operator=(const Pose& p){
    if(&p == this) return *this;

    if(parents.size() != p.parents.size()){
        parents.resize(p.parents.size());
    }
    if(joints.size() != p.joints.size()){
        joints.resize(p.joints.size());
    }

    if(parents.size() != 0){
        memcpy(&parents[0], &p.parents[0], sizeof(int) * parents.size());
    }
    if(joints.size() != 0){
        memcpy(&joints[0], &p.joints[0], sizeof(Transform) * joints.size());
    }

    return *this;
}

void Pose::Resize(unsigned int size){
    parents.resize(size);
    joints.resize(size);
}

unsigned int Pose::Size(){
    return joints.size();
}

Transform Pose::GetLocalTransform(unsigned int index){
    return joints[index];
}

void Pose::SetLocalTransform(unsigned int index, const Transform& transform){
    joints[index] = transform;
}

Transform Pose::GetGlobalTransform(unsigned int i){
    Transform result = joints[i];
    for(int p = parents[i]; p >= 0; p = parents[p]){
        result = Transform::Combine(joints[p], result);
        //result = _joints[p].GetLocalModelMatrix() * result.GetLocalModelMatrix();
    }

    return result;
}

Matrix4 Pose::GetGlobalMatrix(unsigned int i){
    Matrix4 result = joints[i].GetLocalModelMatrix();
    for(int p = parents[i]; p >= 0; p = parents[p]){
        result = joints[p].GetLocalModelMatrix() * result;
    }
    return result;
}

Matrix4 Pose::GetLocalMatrix(unsigned int index){
    return joints[index].GetLocalModelMatrix();
}

Transform Pose::operator[](unsigned int index){
    return GetGlobalTransform(index);
}

void Pose::GetMatrixPalette(std::vector<Matrix4>& out){
    /*
    unsigned int size = Size();
    if(out.size() != size){
        out.reserve(size);
    }
    for(unsigned int i = 0; i < size; ++i){
        Transform t = GetGlobalTransform(i);
        out[i] = t.GetLocalModelMatrix();
    }*/

#if 0
    unsigned int size = Size();
    if(out.size() != size){
        out.reserve(size);
    }
    for(unsigned int i = 0; i < size; ++i){
        out[i] = GetGlobalMatrix(i);
    }

#else
    int size = (int)Size();
    if((int)out.size() != size){ out.resize(size); }
    int i = 0;

    for(; i < size; ++i){
        int parent = parents[i];
        if(parent > i) { break; }
        Matrix4 global = joints[i].GetLocalModelMatrix();
        if(parent >= 0){
            global = out[parent] * global;
        }
        out[i] = global;
    }
    for(; i < size; ++i){
        out[i] = GetGlobalMatrix(i);
    }
#endif
}

void Pose::GetMatrixPalette(std::vector<Matrix4>& out, const std::vector<Matrix4>& invBindPoses){
    unsigned int size = Size();
    if(out.size() != size){
        out.reserve(size);
    }
    for(unsigned int i = 0; i < size; ++i){
        out[i] = GetGlobalMatrix(i) * invBindPoses[i];
    }
}

int Pose::GetParent(unsigned int index){
    return parents[index];
}

void Pose::SetParent(unsigned int index, int parent){
    parents[index] = parent;
}

bool Pose::operator==(const Pose& other) {
	if(joints.size() != other.joints.size()) return false;
	if(parents.size() != other.parents.size()) return false;
	
	unsigned int size = (unsigned int)joints.size();
	for(unsigned int i = 0; i < size; ++i){
		Transform thisLocal = joints[i];
		Transform otherLocal = other.joints[i];

		int thisParent = parents[i];
		int otherParent = other.parents[i];

		if(thisParent != otherParent) return false;
		if(thisLocal != otherLocal) return false;
	}
    
	return true;
}

bool Pose::operator!=(const Pose& other) {
	return !(*this == other);
}

}