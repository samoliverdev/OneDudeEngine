#pragma once

namespace OD{

template<unsigned int N>
class Frame{
public:
    float value[N];
    float in[N];
    float out[N];
    float time;

    template <class Archive>
    void serialize(Archive& ar){
        for(int i = 0; i < N; i++){ ar(value[i]); }
        for(int i = 0; i < N; i++){ ar(in[i]); }
        for(int i = 0; i < N; i++){ ar(out[i]); }
        ar(time);
    }
};

typedef Frame<1> ScalarFrame;
typedef Frame<3> VectorFrame;
typedef Frame<4> QuaternionFrame;

}