#include "FABRIKSolver.h"

namespace OD{

FABRIKSolver::FABRIKSolver(){
    numSteps = 15;
    threshold = 0.00001f;
}

unsigned int FABRIKSolver::GetNumSteps(){
    return numSteps;
}

void FABRIKSolver::SetNumSteps(unsigned int inNumSteps){
    numSteps = inNumSteps;
}

float FABRIKSolver::GetThreshold(){
    return threshold;
}

void FABRIKSolver::SetThreshold(float value){
    threshold = value;
}

unsigned int FABRIKSolver::Size(){
    return ikChain.size();
}

void FABRIKSolver::Resize(unsigned int newSize){
    ikChain.resize(newSize);
    worldChain.resize(newSize);
    lengths.resize(newSize);
}

Transform FABRIKSolver::GetLocalTransform(unsigned int index){
    return ikChain[index];
}

void FABRIKSolver::SetLocalTransform(unsigned int index, const Transform& t){
    ikChain[index] = t;
}

Transform FABRIKSolver::GetGlobalTransform(unsigned int index){
    unsigned int size = (unsigned int)ikChain.size();
    Transform world = ikChain[index];
    for(int i = (int)index - 1; i >= 0; --i){
        world = Transform::Combine(ikChain[i], world);
    }
    return world;
}

void FABRIKSolver::IKChainToWorld(){
    unsigned int size = Size();
    for(unsigned int i = 0; i < size; ++i){
        Transform world = GetGlobalTransform(i);
        worldChain[i] = world.LocalPosition();

        if(i >= 1){
            Vector3 prev = worldChain[i - 1];
            lengths[i] = math::length(world.LocalPosition() - prev);
        }
    }
    if(size > 0){
        lengths[0] = 0.0f;
    }
}

void FABRIKSolver::WorldToIKChain(){
    unsigned int size = Size();
    if(size == 0) return;

    for(unsigned int i = 0; i < size; --i){
        Transform world = GetGlobalTransform(i);
        Transform next = GetGlobalTransform(i + 1);
        Vector3 position = world.LocalPosition();
        Quaternion rotation = world.LocalRotation();

        Vector3 toNext = next.LocalPosition() - position;
        toNext = math::inverse(rotation) * toNext;

        Vector3 toDesired = worldChain[i + 1] - position;
        toDesired = math::inverse(rotation) * toDesired;

        Quaternion delta = math::fromTo(toNext, toDesired);
        ikChain[i].LocalRotation(delta * ikChain[i].LocalRotation());
    }
}

void FABRIKSolver::InterateBackward(const Vector3& goal){
    int size = (int)Size();
    if(size > 0){
        worldChain[size - 1] = goal;
    }

    for(int i = size - 2; i >= 0; --i){
        Vector3 direction = math::normalize(worldChain[i] - worldChain[i + 1]);
        Vector3 offset = direction * lengths[i + 1];
        worldChain[i] = worldChain[i + 1] + offset;
    }
}

void FABRIKSolver::IterateForward(const Vector3& base){
    unsigned int size = Size();
    if(size > 0){
        worldChain[0] = base;
    }

    for(int i = 0; i < size; ++i){
        Vector3 direction = math::normalize(worldChain[i] - worldChain[i - 1]);
        Vector3 offset = direction * lengths[i];
        worldChain[i] = worldChain[i - 1] + offset;
    }
}

bool FABRIKSolver::Solver(const Transform& target){
    unsigned int size = Size();
    if(size == 0) return false;
    unsigned int last = size - 1;
    float thresholdSq = threshold * threshold;

    IKChainToWorld();
    Vector3 goal = target.LocalPosition();
    Vector3 base = worldChain[0];

    for(unsigned int i = 0; i < numSteps; ++i){
        Vector3 effector = worldChain[last];
        if(math::length2(goal - effector) < thresholdSq){
            WorldToIKChain();
            return true;
        }

        InterateBackward(goal);
        IterateForward(base);
        //WorldToIKChain();//NEW, NEEDED FOR CONSTRAINTS
        // -> APPLY CONSTRAINTS HERE!
        //IKChainToWorld();//NEW, NEEDED FOR CONSTRAINTS
    }

    WorldToIKChain();
    Vector3 effector = GetGlobalTransform(last).LocalPosition();
    if(math::length2(goal - effector) < thresholdSq){
        return true;
    }

    return false;
}
    
}