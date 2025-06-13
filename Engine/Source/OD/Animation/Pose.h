#pragma once
#include "OD/Defines.h"
#include "OD/Core/Transform.h"
#include <vector>

namespace OD{

class OD_API Pose{
public:
    Pose();
    Pose(const Pose& p);
    Pose& operator=(const Pose& p);
    Pose(unsigned int numJoints);
    void Resize(unsigned int size);
    unsigned int Size();
    std::vector<int> GetChildrens(unsigned int index);
    int GetParent(unsigned int index);
    void SetParent(unsigned int index, int parent);
    Transform GetLocalTransform(unsigned int index);
    inline Transform& GetLocalTransform2(unsigned int index){ return joints[index]; }
    void SetLocalTransform(unsigned int index, const Transform& transform);
    void SetGlobalTransform(unsigned int index, Transform& transform); //Test
    Transform GetGlobalTransform(unsigned int index);
    Matrix4 GetGlobalMatrix(unsigned int index);
    Matrix4 GetLocalMatrix(unsigned int index);
    Transform operator[](unsigned int index);
    void GetMatrixPalette(AlignedVector<Matrix4>& out);
    void GetMatrixPalette(AlignedVector<Matrix4>& out, const AlignedVector<Matrix4>& invBindPoses);
    bool operator==(const Pose& other);
    bool operator!=(const Pose& other);
    inline void Clear(){ joints.clear(); parents.clear(); }
protected:
    //std::vector<Transform> joints;
    AlignedVector<Transform> joints;
    std::vector<int> parents;
};

}