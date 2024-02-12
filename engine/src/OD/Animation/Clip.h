#pragma once

#include <vector>
#include <string>
#include "TransfomTrack.h"
#include "Pose.h"

namespace OD{

template<typename TRACK>
class TClip{
public:
    TClip();

    unsigned int GetIdAtIndex(unsigned int index);
    void SetIdAtIndex(unsigned int index, unsigned int id);
    unsigned int Size();

    float Sample(Pose& outPose, float time);

    TRACK& operator[](unsigned int joint);

    void RecalculateDuration();

    std::string& GetName();
    void SetName(const std::string& inNewName);
    float GetDuration();
    float GetStartTime();
    float GetEndTime();
    bool GetLooping();
    void SetLooping(bool inLooping);

protected:
    std::vector<TRACK> tracks;
    std::string name;
    float startTime;
    float endTime;
    bool looping;

    float AdjustTimeToFitRange(float inTime);
};

typedef TClip<TransformTrack> Clip;
typedef TClip<FastTransformTrack> FastClip;

FastClip OptimizeClip(Clip& input);

}

#include "Clip.inl"