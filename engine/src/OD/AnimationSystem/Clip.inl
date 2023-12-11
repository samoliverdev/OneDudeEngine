#pragma once
#include "Clip.h"

namespace OD{

template<typename TRACK>
TClip<TRACK>::TClip(){
    name = "No name given";
    startTime = 0;
    endTime = 0;
    looping = true;
}

template<typename TRACK>
unsigned int TClip<TRACK>::GetIdAtIndex(unsigned int index){ 
    return tracks[index].GetId(); 
}

template<typename TRACK>
void TClip<TRACK>::SetIdAtIndex(unsigned int index, unsigned int id){ 
    return tracks[index].SetId(id); 
}

template<typename TRACK>
unsigned int TClip<TRACK>::Size(){ 
    return (unsigned int)tracks.size(); 
}

template<typename TRACK>
float TClip<TRACK>::Sample(Pose& outPose, float time){
    if(GetDuration() == 0) return 0;

    time = AdjustTimeToFitRange(time);

    unsigned int size = tracks.size();
    for(unsigned int i = 0; i < size; ++i){
        unsigned int j = tracks[i].GetId();
        Transform local = outPose.GetLocalTransform(j);
        Transform animated = tracks[i].Sample(local, time, looping);

        if(i == 0){
            animated.LocalPosition(Vector3Zero);
        }

        outPose.SetLocalTransform(j, animated);
    }

    return time;
}

template<typename TRACK>
TRACK& TClip<TRACK>::operator[](unsigned int joint){
    /*if(_tracks.size() < joint+1){
        _tracks.resize(joint+1);
    }
    _tracks[joint].SetId(joint);
    return _tracks[joint];*/

    for(unsigned int i = 0, size = (unsigned int)tracks.size(); i < size; ++i) {
        if(tracks[i].GetId() == joint){
            return tracks[i];
        }
    }

    tracks.push_back(TRACK());
    tracks[tracks.size() - 1].SetId(joint);
    return tracks[tracks.size() - 1];
}

template<typename TRACK>
void TClip<TRACK>::RecalculateDuration(){
    startTime = 0.0f;
    endTime = 0.0f;
    bool startSet = false;
    bool endSet = false;
    unsigned int tracksSize = (unsigned int)tracks.size();
    for(unsigned int i = 0; i < tracksSize; ++i){
        if(tracks[i].IsValid()){
            float trackStartTime = tracks[i].GetStartTime();
            float trackEndTime = tracks[i].GetEndTime();

            if(trackStartTime < startTime || !startSet){
                startTime = trackStartTime;
                startSet = true;
            }

            if(trackEndTime > endTime || !endSet){
                endTime = trackEndTime;
                endSet = true;
            }
        }
    }
}

template<typename TRACK>
std::string& TClip<TRACK>::GetName(){ 
    return name; 
}

template<typename TRACK>
void TClip<TRACK>::SetName(const std::string& inNewName){ 
    name = inNewName; 
}

template<typename TRACK>
float TClip<TRACK>::GetDuration(){ 
    return endTime - startTime; 
}

template<typename TRACK>
float TClip<TRACK>::GetStartTime(){ 
    return startTime; 
}

template<typename TRACK>
float TClip<TRACK>::GetEndTime(){ 
    return endTime; 
}

template<typename TRACK>
bool TClip<TRACK>::GetLooping(){ 
    return looping; 
}

template<typename TRACK>
void TClip<TRACK>::SetLooping(bool inLooping){ 
    looping = inLooping; 
}

template<typename TRACK>
float TClip<TRACK>::AdjustTimeToFitRange(float inTime){
    if(looping) {
        float duration = endTime - startTime;
        if(duration <= 0) return 0.0f;
        
        inTime = fmodf(inTime - startTime, endTime - startTime);
        if(inTime < 0.0f){
            inTime += endTime - startTime;
        }
        inTime = inTime + startTime;
    } else {
        if(inTime < startTime){
            inTime = startTime;
        }
        if(inTime > endTime){
            inTime = endTime;
        }
    }
    return inTime;
}

}