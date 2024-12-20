#pragma once
#include "OD/Defines.h"
#include "OD/Core/Transform.h"

namespace OD{

class CCDSolver{
public:
    CCDSolver();
    
    unsigned int Size();
    void Resize(unsigned int newSize);

    Transform& operator[](unsigned int index);
    Transform GetGlobalTransform(unsigned int index);
    
    unsigned int GetNumSteps();
    void SetNumSteps(unsigned int numSteps);

    float GetThreshold();
    void SetThreshold(float value);

    bool Solver(const Transform& target);

protected:
    std::vector<Transform> ikChain;
    unsigned int numSteps;
    float threshold;
};

}