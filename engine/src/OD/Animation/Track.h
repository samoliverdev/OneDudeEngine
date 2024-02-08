#pragma once

#include <vector>
#include "OD/Core/Math.h"
#include "Frame.h"
#include "Interpolation.h"

namespace OD{

template<typename T, int N>
class Track{
public: 
    Track();
    
    void Resize(unsigned int size);
    unsigned int Size();
    Interpolation GetInterpolation();
    void SetInterpolation(Interpolation interp);
    float GetStartTime();
    float GetEndTime();

    T Sample(float time, bool looping);

    Frame<N>& operator[](unsigned int index);

protected:
    std::vector<Frame<N>> frames;
    Interpolation interpolation;

    T SampleConstant(float t, bool looping);
    T SampleLinear(float time, bool looping);
    T SampleCubic(float time, bool looping);

    T Hermite(float t, const T& p1, const T& s1, const T& _p2, const T& s2);

    virtual int FrameIndex(float time, bool looping);

    float AdjustTimeToFitTrack(float time, bool looping);

    T Cast(float* value); // Will be specialized
};

typedef Track<float, 1> ScalarTrack;
typedef Track<Vector3, 3> VectorTrack;
typedef Track<Quaternion, 4> QuaternionTrack;

template<typename T, int N>
class FastTrack: public Track<T, N>{
public:
    void UpdateIndexLookupTable();

protected:
    std::vector<unsigned int> sampledFrames;
    virtual int FrameIndex(float time, bool looping);
};

typedef FastTrack<float, 1> FastScalarTrack;
typedef FastTrack<Vector3, 3> FastVectorTrack;
typedef FastTrack<Quaternion, 4> FastQuaternionTrack;

template<typename T, int N>
FastTrack<T, N> OptimizeTrack(Track<T, N>& input);

template FastTrack<float, 1> OptimizeTrack(Track<float, 1>& input);
template FastTrack<Vector3, 3> OptimizeTrack(Track<Vector3, 3>& input);
template FastTrack<Quaternion, 4> OptimizeTrack(Track<Quaternion, 4>& input);

}

#include "Track.inl"