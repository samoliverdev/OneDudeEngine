#pragma once
#include "OD/Defines.h"
#include "TransfomTrack.h"
#include "Pose.h"
#include <vector>
#include <string>

namespace OD{

template<typename TRACK>
class TClip{
public:
    TClip();

    unsigned int GetIdAtIndex(unsigned int index);
    void SetIdAtIndex(unsigned int index, unsigned int id);
    unsigned int Size();

    float Sample(Pose& outPose, float time, int rootMotionIndex = -1, Vector3 rootMotionPosMask = {1, 1, 0}, Transform* outDelta = nullptr);

    TRACK& operator[](unsigned int joint);

    void RecalculateDuration();

    std::string& GetName();
    void SetName(const std::string& inNewName);
    float GetDuration();
    float GetStartTime();
    float GetEndTime();
    bool GetLooping();
    void SetLooping(bool inLooping);
    bool GetHasRootMotion();
    void SetHasRootMotion(bool v);

    template <class Archive>
    void serialize(Archive& ar){
        ar(tracks);
        ar(name);
        ar(startTime);
        ar(endTime);
        ar(looping);
    }

protected:
    std::vector<TRACK> tracks;
    std::string name;
    float startTime;
    float endTime;
    bool looping;
    bool hasRootMotion = false;

    float AdjustTimeToFitRange(float inTime);
};

typedef TClip<TransformTrack> Clip;
typedef TClip<FastTransformTrack> FastClip;

FastClip OD_API OptimizeClip(Clip& input);

//#define OptimizeClipT(arg) OptimizeClip(arg)
//using ClipT = FastClip; //TODO: This is bug, Check this later

#define OptimizeClipT(arg) arg
using ClipT = Clip;

}

#include "Clip.inl"