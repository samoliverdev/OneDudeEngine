#pragma once

#include <vector>
#include <string>
#include "TransfomTrack.h"
#include "Pose.h"

namespace OD{

class Clip{
public:
    Clip();

    unsigned int GetIdAtIndex(unsigned int index);
    void SetIdAtIndex(unsigned int idx, unsigned int id);
    unsigned int Size();

    float Sample(Pose& out, float inTime);
    TransformTrack& operator[](unsigned int index);

    void RecalculateDuration();

    std::string& GetName();
    void SetName(const std::string& inNewName);
    float GetDuration();
    float GetStartTime();
    float GetEndTime();
    bool GetLooping();
    void SetLooping(bool inLooping);

protected:
    std::vector<TransformTrack> _tracks;
    std::string _name;
    float _startTime;
    float _endTime;
    bool _looping;

    float AdjustTimeToFitRange(float inTime);
};

}