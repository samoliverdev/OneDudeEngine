#pragma once

#include "OD/Core/Transform.h"
#include "Track.h"

namespace OD{

class TransformTrack{
public:
    TransformTrack();
    unsigned int GetId();
    void SetId(unsigned int id);
    VectorTrack& GetPositionTrack();
    QuaternionTrack& GetRotationTrack();
    VectorTrack& GetScaleTrack();
    float GetStartTime();
    float GetEndTime();
    bool IsValid();
    Transform Sample(const Transform& ref, float time, bool looping);
    
protected:
    unsigned int _id;
    VectorTrack _position;
    QuaternionTrack _rotation;
    VectorTrack _scale;
};

}