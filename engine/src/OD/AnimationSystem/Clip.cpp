#include "Clip.h"

namespace OD{

Clip::Clip(){
    _name = "No name given";
    _startTime = 0;
    _endTime = 0;
    _looping = true;
}

float Clip::Sample(Pose& outPose, float time){
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

float Clip::AdjustTimeToFitRange(float inTime){
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

void Clip::RecalculateDuration(){
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

TransformTrack& Clip::operator[](unsigned int joint){
	for(unsigned int i = 0, size = (unsigned int)_tracks.size(); i < size; ++i) {
		if(_tracks[i].GetId() == joint){
			return _tracks[i];
		}
	}

	_tracks.push_back(TransformTrack());
	_tracks[_tracks.size() - 1].SetId(joint);
	return _tracks[_tracks.size() - 1];
}

std::string& Clip::GetName(){ return _name; }
void Clip::SetName(const std::string& inNewName){ _name = inNewName; }

unsigned int Clip::GetIdAtIndex(unsigned int index){ return _tracks[index].GetId(); }
void Clip::SetIdAtIndex(unsigned int index, unsigned int id){ return _tracks[index].SetId(id); }

unsigned int Clip::Size(){ return (unsigned int)_tracks.size(); }
float Clip::GetDuration(){ return _endTime - _startTime; }
float Clip::GetStartTime(){ return _startTime; }
float Clip::GetEndTime() {return _endTime; }
bool Clip::GetLooping(){ return _looping; }
void Clip::SetLooping(bool inLooping){ _looping = inLooping; }

}