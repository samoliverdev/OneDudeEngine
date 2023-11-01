#pragma once

#include <vector>
#include "OD/Core/Math.h"
#include "Frame.h"
#include "Interpolation.h"

namespace OD{

template<typename T, int N>
class Track{
public: 
    Track(){ _interpolation =Interpolation::Linear; }
    
    void Resize(unsigned int size){ _frames.resize(size); }
    unsigned int Size(){ return _frames.size(); }
    Interpolation GetInterpolation(){ return _interpolation; }
    void SetInterpolation(Interpolation interp){ _interpolation = interp; }
    float GetStartTime(){ return _frames[0].time; }
    float GetEndTime(){ return _frames[_frames.size()-1].time; }
    
    T Sample(float time, bool looping){
        if(_interpolation == Interpolation::Constant){
            return SampleConstant(time, looping);
        } else if(_interpolation == Interpolation::Linear){
            return SampleLinear(time, looping);
        } 
        
        return SampleCubic(time, looping);
    }

    Frame<N>& operator[](unsigned int index){ return _frames[index]; }

protected:
    std::vector<Frame<N>> _frames;
    Interpolation _interpolation;

    T SampleConstant(float t, bool looping){
        int frame = FrameIndex(t, looping);
        if(frame < 0 || frame >= (int)_frames.size()){
            return T();
        }

        return Cast(&_frames[frame].value[0]);
    }

    T SampleLinear(float time, bool looping){
        int thisFrame = FrameIndex(time, looping);
        if(thisFrame < 0 || thisFrame >= (int)(_frames.size() - 1)) return T();
        
        int nextFrame = thisFrame + 1;

        float trackTime = AdjustTimeToFitTrack(time, looping);
        float frameDelta = _frames[nextFrame].time - _frames[thisFrame].time;
        if(frameDelta <= 0.0f) return T();

        float t = (trackTime - _frames[thisFrame].time) / frameDelta;

        T start = Cast(&_frames[thisFrame].value[0]);
        T end = Cast(&_frames[nextFrame].value[0]);

        return Interpolate(start, end, t);
    }

    T SampleCubic(float time, bool looping){
        int thisFrame = FrameIndex(time, looping);
        if(thisFrame < 0 || thisFrame >= (int)(_frames.size() - 1)) return T();

        int nextFrame = thisFrame + 1;

        float trackTime = AdjustTimeToFitTrack(time, looping);
        float frameDelta = _frames[nextFrame].time - _frames[thisFrame].time;
        if(frameDelta <= 0.0f) return T();
        
        float t = (trackTime - _frames[thisFrame].time) / frameDelta;

        T point1 = Cast(&_frames[thisFrame].value[0]);
        T slope1;// = mFrames[thisFrame].mOut * frameDelta;
        memcpy(&slope1, _frames[thisFrame].out, N * sizeof(float));
        slope1 = slope1 * frameDelta;

        T point2 = Cast(&_frames[nextFrame].value[0]);
        T slope2;// = mFrames[nextFrame].mIn[0] * frameDelta;
        memcpy(&slope2, _frames[nextFrame].in, N * sizeof(float));
        slope2 = slope2 * frameDelta;

        return Hermite(t, point1, slope1, point2, slope2);
    }

    T Hermite(float t, const T& p1, const T& s1, const T& _p2, const T& s2){
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

    int FrameIndex(float time, bool looping){
        unsigned int size = (unsigned int)_frames.size();
        if(size <= 1) return -1;

        if(looping){
            float startTime = _frames[0].time;
            float endTime = _frames[size - 1].time;
            float duration = endTime - startTime;

            time = fmodf(time - startTime, endTime - startTime);
            if(time < 0.0f){
                time += endTime - startTime;
            }
            time = time + startTime;
        } else {
            if(time <= _frames[0].time){
                return 0;
            }
            if(time >= _frames[size - 2].time){
                return (int)size - 2;
            }
        }

        for(int i = (int)size -1; i >= 0; --i){
            if(time >= _frames[i].time) return i;
        }

        return -1;
    }

    float AdjustTimeToFitTrack(float time, bool looping){
        unsigned int size = (unsigned int)_frames.size();
        if(size <= 1) return 0.0f;

        float startTime = _frames[0].time;
        float endTime = _frames[size-1].time;
        float duration = endTime - startTime;
        
        if(duration <= 0.0f) return 0.0f;

        if(looping){
            time = fmodf(time - startTime, endTime - startTime);
            if(time <= 0.0f){
                time += endTime - startTime;
            }
            time = time + startTime;
        } else {
            if(time <= _frames[0].time){
                time = startTime;
            }
            if(time >= _frames[size-1].time){
                time = endTime;
            }
        }

        return time;
    }

    T Cast(float* value); // Will be specialized

private:
    inline float Interpolate(float a, float b, float t){ return a + (b - a) * t; }
    inline Vector3 Interpolate(Vector3 a, Vector3 b, float t){ return math::mix(a, b, t); }

    inline Quaternion Interpolate(Quaternion a, Quaternion b, float t){
        //return math::lerp(a, b, t);

        Quaternion result = math::lerp(a, b, t);
        if(math::dot(a, b) < 0){
            result = math::lerp(a, -b, t);
        }
        return math::normalize(result);
    }

    inline float AdjustHermiteResult(float f){ return f; }
    inline Vector3 AdjustHermiteResult(const Vector3& v){ return v; }
    inline Quaternion AdjustHermiteResult(const Quaternion& q){ return math::normalize(q); }

    inline void Neighborhood(const float& a, float& b){}
    inline void Neighborhood(const Vector3& a, Vector3& b){}
    inline void Neighborhood(const Quaternion& a, Quaternion& b){
        if(dot(a, b) < 0){
            b = -b;
        }
    }
};

typedef Track<float, 1> ScalarTrack;
typedef Track<Vector3, 3> VectorTrack;
typedef Track<Quaternion, 4> QuaternionTrack;

}