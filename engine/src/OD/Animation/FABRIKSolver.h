#pragma once
#include "OD/Defines.h"
#include "OD/Core/Transform.h"


namespace OD{

class FABRIKSolver{
public:
    FABRIKSolver();

    unsigned int Size();
    void Resize(unsigned int newSize);

    unsigned int GetNumSteps();
    void SetNumSteps(unsigned int numSteps);

    float GetThreshold();
    void SetThreshold(float value);

    Transform GetLocalTransform(unsigned int index);
    void SetLocalTransform(unsigned int index, const Transform& t);
    Transform GetGlobalTransform(unsigned int index);

    bool Solver(const Transform& target);

protected:
    std::vector<Transform> ikChain;
    unsigned int numSteps;
    float threshold;
    std::vector<Vector3> worldChain;
    std::vector<float> lengths;

    void IKChainToWorld();
    void IterateForward(const Vector3& goal);
    void InterateBackward(const Vector3& base);
    void WorldToIKChain();
};

}