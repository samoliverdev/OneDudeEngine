#pragma once
#include "Track.h"

namespace OD{

template<typename T, int N>
Track<T,N>::Track(){ 
    interpolation = Interpolation::Linear; 
}

template<typename T, int N>
void Track<T,N>::Resize(unsigned int size){ 
    frames.resize(size); 
}

template<typename T, int N>
unsigned int Track<T,N>::Size(){ 
    return frames.size(); 
}

template<typename T, int N>
Interpolation Track<T,N>::GetInterpolation(){ 
    return interpolation; 
}

template<typename T, int N>
void Track<T,N>::SetInterpolation(Interpolation interp){ 
    interpolation = interp; 
}

template<typename T, int N>
float Track<T,N>::GetStartTime(){ 
    return frames[0].time; 
}

template<typename T, int N>
float Track<T,N>::GetEndTime(){ 
    return frames[frames.size()-1].time; 
}

template<typename T, int N>
T Track<T,N>::Sample(float time, bool looping){
    if(interpolation == Interpolation::Constant){
        return SampleConstant(time, looping);
    } else if(interpolation == Interpolation::Linear){
        return SampleLinear(time, looping);
    } 
    
    return SampleCubic(time, looping);
}

template<typename T, int N>
Frame<N>& Track<T,N>::operator[](unsigned int index){ 
    return frames[index]; 
}

template<typename T, int N>
T Track<T,N>::SampleConstant(float t, bool looping){
    int frame = FrameIndex(t, looping);
    if(frame < 0 || frame >= (int)frames.size()){
        return T();
    }

    return Cast(&frames[frame].value[0]);
}

template<typename T, int N>
T Track<T,N>::SampleLinear(float time, bool looping){
    int thisFrame = FrameIndex(time, looping);
    if(thisFrame < 0 || thisFrame >= (int)(frames.size() - 1)) return T();
    
    int nextFrame = thisFrame + 1;

    float trackTime = AdjustTimeToFitTrack(time, looping);
    float frameDelta = frames[nextFrame].time - frames[thisFrame].time;
    if(frameDelta <= 0.0f) return T();

    float t = (trackTime - frames[thisFrame].time) / frameDelta;

    T start = Cast(&frames[thisFrame].value[0]);
    T end = Cast(&frames[nextFrame].value[0]);

    return Interpolate(start, end, t);
}

template<typename T, int N>
T Track<T,N>::SampleCubic(float time, bool looping){
    int thisFrame = FrameIndex(time, looping);
    if(thisFrame < 0 || thisFrame >= (int)(frames.size() - 1)) return T();

    int nextFrame = thisFrame + 1;

    float trackTime = AdjustTimeToFitTrack(time, looping);
    float frameDelta = frames[nextFrame].time - frames[thisFrame].time;
    if(frameDelta <= 0.0f) return T();
    
    float t = (trackTime - frames[thisFrame].time) / frameDelta;

    T point1 = Cast(&frames[thisFrame].value[0]);
    T slope1;// = mFrames[thisFrame].mOut * frameDelta;
    memcpy(&slope1, frames[thisFrame].out, N * sizeof(float));
    slope1 = slope1 * frameDelta;

    T point2 = Cast(&frames[nextFrame].value[0]);
    T slope2;// = mFrames[nextFrame].mIn[0] * frameDelta;
    memcpy(&slope2, frames[nextFrame].in, N * sizeof(float));
    slope2 = slope2 * frameDelta;

    return Hermite(t, point1, slope1, point2, slope2);
}

template<typename T, int N>
T Track<T,N>::Hermite(float t, const T& p1, const T& s1, const T& _p2, const T& s2){
    float tt = t * t;
    float ttt = tt * t;

    T p2 = _p2;
    Neighborhood(p1, p2);

    float h1 = 2.0f * ttt - 3.0f * tt + 1.0f;
    float h2 = -2.0f * ttt + 3.0f * tt;
    float h3 = ttt - 2.0f * tt + t;
    float h4 = ttt - tt;
    T result = p1 * h1 + p2 * h2 + s1 * h3 + s2 * h4;
    return AdjustHermiteResult(result);
}

template<typename T, int N>
int Track<T,N>::FrameIndex(float time, bool looping){
    unsigned int size = (unsigned int)frames.size();
    if(size <= 1) return -1;

    if(looping){
        float startTime = frames[0].time;
        float endTime = frames[size - 1].time;
        float duration = endTime - startTime;

        time = fmodf(time - startTime, endTime - startTime);
        if(time < 0.0f){
            time += endTime - startTime;
        }
        time = time + startTime;
    } else {
        if(time <= frames[0].time){
            return 0;
        }
        if(time >= frames[size - 2].time){
            return (int)size - 2;
        }
    }

    for(int i = (int)size -1; i >= 0; --i){
        if(time >= frames[i].time) return i;
    }

    return -1;
}

template<typename T, int N>
float Track<T,N>::AdjustTimeToFitTrack(float time, bool looping){
    unsigned int size = (unsigned int)frames.size();
    if(size <= 1) return 0.0f;

    float startTime = frames[0].time;
    float endTime = frames[size-1].time;
    float duration = endTime - startTime;
    
    if(duration <= 0.0f) return 0.0f;

    if(looping){
        time = fmodf(time - startTime, endTime - startTime);
        if(time <= 0.0f){
            time += endTime - startTime;
        }
        time = time + startTime;
    } else {
        if(time <= frames[0].time){
            time = startTime;
        }
        if(time >= frames[size-1].time){
            time = endTime;
        }
    }

    return time;
}

template<typename T, int N>
float Track<T,N>::Interpolate(float a, float b, float t){
    t = math::clamp<float>(t, 0, 1);
    return math::mix(a, b, t);
}

template<typename T, int N>
Vector3 Track<T,N>::Interpolate(Vector3 a, Vector3 b, float t){ 
    t = math::clamp<float>(t, 0, 1);
    return math::mix(a, b, t);
}

template<typename T, int N>
Quaternion Track<T,N>::Interpolate(Quaternion a, Quaternion b, float t){
    t = math::clamp<float>(t, 0, 1);
    Quaternion result = math::lerp(a, b, t);
    /*if(math::dot(a, b) < 0){
        result = math::slerp(a, -b, t);
    }*/
    return math::normalize(result);
    return result;
}

template<typename T, int N>
float Track<T,N>::AdjustHermiteResult(float f){ 
    return f; 
}

template<typename T, int N>
Vector3 Track<T,N>::AdjustHermiteResult(const Vector3& v){ 
    return v; 
}

template<typename T, int N>
Quaternion Track<T,N>::AdjustHermiteResult(const Quaternion& q){ 
    return math::normalize(q); 
    //return q;
}

template<typename T, int N>
void Track<T,N>::Neighborhood(const float& a, float& b){}

template<typename T, int N>
void Track<T,N>::Neighborhood(const Vector3& a, Vector3& b){}

template<typename T, int N>
void Track<T,N>::Neighborhood(const Quaternion& a, Quaternion& b){
    /*if(dot(a, b) < 0){
        b = -b;
    }*/
}

//-------------FastTrack------------

template<typename T, int N>
int FastTrack<T,N>::FrameIndex(float time, bool looping){
    std::vector<Frame<N>>& frames = this->frames;

    unsigned int size = (unsigned int)frames.size();
    if(size <= 1){ return -1; }

    if(looping){
        float startTime = frames[0].time;
        float endTime = frames[size - 1].time;
        float duration = endTime - startTime;
        while(time < startTime){ time += duration; }
        while(time > endTime){ time -= duration; }
        if(time == endTime){ time = startTime; }
    } else {
        if(time <= frames[0].time){
            return 0;
        }
        if(time >= frames[size - 2].time){
            return (int)size - 2;
        }
    }
    float duration = this->GetEndTime() - this->GetStartTime();
    unsigned int numSamples = 60 + (unsigned int)(duration * 60.0f);
    float t = time / duration;

    unsigned int index = (unsigned int)(t * (float)numSamples);
    if(index >= sampledFrames.size()){
        return -1;
    }
    return (int)sampledFrames[index];
}

template<typename T, int N>
void FastTrack<T,N>::UpdateIndexLookupTable(){
    int numFrames = (int)this->frames.size();
    if(numFrames <= 1){
        return;
    }

    float duration = this->GetEndTime() - this->GetStartTime();
    unsigned int numSamples = 60 + (unsigned int)(duration * 60.0f);
    sampledFrames.resize(numSamples);
    for(unsigned int i = 0; i < numSamples; ++i) {
        float t = (float)i / (float)(numSamples - 1);
        float time = t * duration + this->GetStartTime();

        unsigned int frameIndex = 0;
        for (int j = numFrames - 1; j >= 0; --j) {
            if (time >= this->frames[j].time) {
                frameIndex = (unsigned int)j;
                if ((int)frameIndex >= numFrames - 2) {
                    frameIndex = numFrames - 2;
                }
                break;
            }
        }
        sampledFrames[i] = frameIndex;
    }
}

template<typename T, int N>
FastTrack<T, N> OptimizeTrack(Track<T, N>& input){
    FastTrack<T, N> result;

	result.SetInterpolation(input.GetInterpolation());
	unsigned int size = input.Size();
	result.Resize(size);
	for(unsigned int i = 0; i < size; ++i){
		result[i] = input[i];
	}
	result.UpdateIndexLookupTable();

	return result;
}

}