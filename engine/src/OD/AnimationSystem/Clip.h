#pragma once

#include <vector>
#include <string>
#include "TransfomTrack.h"
#include "Pose.h"

namespace OD{

template <typename TRACK>
class TClip{
public:
    TClip(){
        _name = "No name given";
        _startTime = 0;
        _endTime = 0;
        _looping = true;
    }

    unsigned int GetIdAtIndex(unsigned int index){ return _tracks[index].GetId(); }
    void SetIdAtIndex(unsigned int index, unsigned int id){ return _tracks[index].SetId(id); }
    unsigned int Size(){ return (unsigned int)_tracks.size(); }

    float Sample(Pose& outPose, float time){
        if(GetDuration() == 0) return 0;

        time = AdjustTimeToFitRange(time);

        unsigned int size = _tracks.size();
        for(unsigned int i = 0; i < size; ++i){
            unsigned int j = _tracks[i].GetId(); // Joint
            Transform local = outPose.GetLocalTransform(j);
            Transform animated = _tracks[i].Sample(local, time, _looping);
            outPose.SetLocalTransform(j, animated);
        }

        return time;
    }

    TRACK& operator[](unsigned int joint){
        for(unsigned int i = 0, size = (unsigned int)_tracks.size(); i < size; ++i) {
            if(_tracks[i].GetId() == joint){
                return _tracks[i];
            }
        }

        _tracks.push_back(TRACK());
        _tracks[_tracks.size() - 1].SetId(joint);
        return _tracks[_tracks.size() - 1];
    }

    void RecalculateDuration(){
        _startTime = 0.0f;
        _endTime = 0.0f;
        bool startSet = false;
        bool endSet = false;
        unsigned int tracksSize = (unsigned int)_tracks.size();
        for(unsigned int i = 0; i < tracksSize; ++i){
            if(_tracks[i].IsValid()){
                float trackStartTime = _tracks[i].GetStartTime();
                float trackEndTime = _tracks[i].GetEndTime();

                if(trackStartTime < _startTime || !startSet){
                    _startTime = trackStartTime;
                    startSet = true;
                }

                if(trackEndTime > _endTime || !endSet){
                    _endTime = trackEndTime;
                    endSet = true;
                }
            }
        }
    }

    std::string& GetName(){ return _name; }
    void SetName(const std::string& inNewName){ _name = inNewName; }
    float GetDuration(){ return _endTime - _startTime; }
    float GetStartTime(){ return _startTime; }
    float GetEndTime(){ return _endTime; }
    bool GetLooping(){ return _looping; }
    void SetLooping(bool inLooping){ _looping = inLooping; }

protected:
    std::vector<TRACK> _tracks;
    std::string _name;
    float _startTime;
    float _endTime;
    bool _looping;

    float AdjustTimeToFitRange(float inTime){
        if(_looping) {
            float duration = _endTime - _startTime;
            if(duration <= 0) return 0.0f;
            
            inTime = fmodf(inTime - _startTime, _endTime - _startTime);
            if(inTime < 0.0f){
                inTime += _endTime - _startTime;
            }
            inTime = inTime + _startTime;
        } else {
            if(inTime < _startTime){
                inTime = _startTime;
            }
            if(inTime > _endTime){
                inTime = _endTime;
            }
        }
        return inTime;
    }
};

typedef TClip<TransformTrack> Clip;
typedef TClip<FastTransformTrack> FastClip;

FastClip OptimizeClip(Clip& input);

}