#include "CCDSolver.h"

namespace OD{

CCDSolver::CCDSolver(){
    numSteps = 15;
    threshold = 0.00001f;
}
    
unsigned int CCDSolver::Size(){
    return ikChain.size();
}

void CCDSolver::Resize(unsigned int newSize){
    ikChain.resize(newSize);
}

Transform& CCDSolver::operator[](unsigned int index){
    return ikChain[index];
}

unsigned int CCDSolver::GetNumSteps(){
    return numSteps;
}

void CCDSolver::SetNumSteps(unsigned int inNumSteps){
    numSteps = inNumSteps;
}

float CCDSolver::GetThreshold(){
    return threshold;
}

void CCDSolver::SetThreshold(float value){
    threshold = value;
}

Transform CCDSolver::GetGlobalTransform(unsigned int x){
    unsigned int size = (unsigned int)ikChain.size();
    Transform world = ikChain[x];
    for(int i = (int)x - 1; i >= 0; --i){
        world = Transform::Combine(ikChain[i], world);
    }
    return world;
}

bool CCDSolver::Solver(const Transform& target){
    unsigned int size = Size();
    if(size == 0) return false;

    unsigned int last = size - 1;
    float thresholdSq = threshold * threshold;
    Vector3 goal = target.LocalPosition();

    for(unsigned int i = 0; i < numSteps; ++i){
        Vector3 effector = GetGlobalTransform(last).LocalPosition();

        if(math::length2(goal - effector) < thresholdSq) return true;

        for(int j = (int)size - 2; j >= 0; --j){
            // Iteration logic
            // -> APPLY CONSTRAINTS HERE!
            effector = GetGlobalTransform(last).LocalPosition();

            Transform world = GetGlobalTransform(j);
            Vector3 position = world.LocalPosition();
            Quaternion rotation = world.LocalRotation();

            Vector3 toEffector = effector - position;
            Vector3 toGoal = goal - position;

            Quaternion effectorToGoal;
            if(math::length2(toGoal) > 0.00001f){
                effectorToGoal = math::fromTo(toEffector, toGoal);
            }

            Quaternion worldRotated = rotation * effectorToGoal;
            Quaternion localRotate = worldRotated * math::inverse(rotation);

            ikChain[j].LocalRotation(localRotate * ikChain[j].LocalRotation());

            effector = GetGlobalTransform(last).LocalPosition();
            if(math::length2(goal - effector) < thresholdSq) return true;
        }
    }

    return false;
}


}