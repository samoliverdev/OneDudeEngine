#pragma once

#include "OD/Core/Transform.h"
#include "Track.h"

namespace OD{

template <typename VTRACK, typename QTRACK>
class TTransformTrack{
public:
    TTransformTrack(){ id = 0; }
    unsigned int GetId(){ return id; }
    void SetId(unsigned int newId){ id = newId; }
    VTRACK& GetPositionTrack(){ return position; }
    QTRACK& GetRotationTrack(){ return rotation; }
    VTRACK& GetScaleTrack(){ return scale; }
    
    float GetStartTime(){
        float result = 0.0f;
        bool isSet = false;

        if(position.Size() > 1){
            result = position.GetStartTime();
            isSet = true;
        }
        if(rotation.Size() > 1){
            float rotationStart = rotation.GetStartTime();
            if(rotationStart < result || !isSet){
                result = rotationStart;
                isSet = true;
            }
        }
        if(scale.Size() > 1){
            float scaleStart = scale.GetStartTime();
            if(scaleStart < result || !isSet){
                result = scaleStart;
                isSet = true;
            }
        }

        return result;
    }

    float GetEndTime(){
        float result = 0.0f;
        bool isSet = false;

        if(position.Size() > 1){
            result = position.GetEndTime();
            isSet = true;
        }
        if (rotation.Size() > 1){
            float rotationEnd = rotation.GetEndTime();
            if (rotationEnd > result || !isSet) {
                result = rotationEnd;
                isSet = true;
            }
        }
        if(scale.Size() > 1){
            float scaleEnd = scale.GetEndTime();
            if (scaleEnd > result || !isSet) {
                result = scaleEnd;
                isSet = true;
            }
        }

        return result;
    }

    bool IsValid(){
        return position.Size() > 1 || rotation.Size() > 1 || scale.Size() > 1;
    }

    Transform Sample(const Transform& ref, float time, bool looping){
        Transform result = ref; // Assign default values
        
        if(position.Size() > 1){ // Only assign if animated
            result.LocalPosition(position.Sample(time, looping));
        }
        if(rotation.Size() > 1){ // Only assign if animated
            result.LocalRotation(rotation.Sample(time, looping));
        }
        if(scale.Size() > 1){ // Only assign if animated
            result.LocalScale(scale.Sample(time, looping));
        }
        return result;
    }
    
protected:
    unsigned int id;
    VTRACK position;
    QTRACK rotation;
    VTRACK scale;
};

typedef TTransformTrack<VectorTrack, QuaternionTrack> TransformTrack;
typedef TTransformTrack<FastVectorTrack, FastQuaternionTrack> FastTransformTrack;

//template<> class TTransformTrack<VectorTrack, QuaternionTrack>;
//template<> class TTransformTrack<FastVectorTrack, FastQuaternionTrack>;

FastTransformTrack OptimizeTransformTrack(TransformTrack& input);

}