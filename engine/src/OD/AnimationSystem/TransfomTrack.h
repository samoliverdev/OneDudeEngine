#pragma once

#include "OD/Core/Transform.h"
#include "Track.h"

namespace OD{

template <typename VTRACK, typename QTRACK>
class TTransformTrack{
public:
    TTransformTrack(){ _id = 0; }
    unsigned int GetId(){ return _id; }
    void SetId(unsigned int id){ _id = id; }
    VTRACK& GetPositionTrack(){ return _position; }
    QTRACK& GetRotationTrack(){ return _rotation; }
    VTRACK& GetScaleTrack(){ return _scale; }
    
    float GetStartTime(){
        float result = 0.0f;
        bool isSet = false;

        if(_position.Size() > 1){
            result = _position.GetStartTime();
            isSet = true;
        }
        if(_rotation.Size() > 1){
            float rotationStart = _rotation.GetStartTime();
            if(rotationStart < result || !isSet){
                result = rotationStart;
                isSet = true;
            }
        }
        if(_scale.Size() > 1){
            float scaleStart = _scale.GetStartTime();
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

        if(_position.Size() > 1){
            result = _position.GetEndTime();
            isSet = true;
        }
        if (_rotation.Size() > 1){
            float rotationEnd = _rotation.GetEndTime();
            if (rotationEnd > result || !isSet) {
                result = rotationEnd;
                isSet = true;
            }
        }
        if(_scale.Size() > 1){
            float scaleEnd = _scale.GetEndTime();
            if (scaleEnd > result || !isSet) {
                result = scaleEnd;
                isSet = true;
            }
        }

        return result;
    }

    bool IsValid(){
        return _position.Size() > 1 || _rotation.Size() > 1 || _scale.Size() > 1;
    }

    Transform Sample(const Transform& ref, float time, bool looping){
        Transform result = ref; // Assign default values
        if(_position.Size() > 1){ // Only assign if animated
            result.localPosition(_position.Sample(time, looping));
        }
        if(_rotation.Size() > 1){ // Only assign if animated
            result.localRotation(_rotation.Sample(time, looping));
        }
        if(_scale.Size() > 1){ // Only assign if animated
            result.localScale(_scale.Sample(time, looping));
        }
        return result;
    }
    
protected:
    unsigned int _id;
    VTRACK _position;
    QTRACK _rotation;
    VTRACK _scale;
};

typedef TTransformTrack<VectorTrack, QuaternionTrack> TransformTrack;
typedef TTransformTrack<FastVectorTrack, FastQuaternionTrack> FastTransformTrack;

//template<> class TTransformTrack<VectorTrack, QuaternionTrack>;
//template<> class TTransformTrack<FastVectorTrack, FastQuaternionTrack>;

FastTransformTrack OptimizeTransformTrack(TransformTrack& input);

}