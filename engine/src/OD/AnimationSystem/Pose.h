#pragma once

#include <vector>
#include "OD/Core/Transform.h"

namespace OD{

class Pose{
public:
    Pose();
    Pose(const Pose& p);
    Pose& operator=(const Pose& p);
    Pose(unsigned int numJoints);
    void Resize(unsigned int size);
    unsigned int Size();
    int GetParent(unsigned int index);
    void SetParent(unsigned int index, int parent);
    Transform GetLocalTransform(unsigned int index);
    inline Transform& GetLocalTransform2(unsigned int index){ return joints[index]; }
    void SetLocalTransform(unsigned int index, const Transform& transform);
    Transform GetGlobalTransform(unsigned int index);
    Matrix4 GetGlobalMatrix(unsigned int index);
    Matrix4 GetLocalMatrix(unsigned int index);
    Transform operator[](unsigned int index);
    void GetMatrixPalette(std::vector<Matrix4>& out);
    void GetMatrixPalette(std::vector<Matrix4>& out, const std::vector<Matrix4>& invBindPoses);
    bool operator==(const Pose& other);
    bool operator!=(const Pose& other);
protected:
    std::vector<Transform> joints;
    std::vector<int> parents;
};

}