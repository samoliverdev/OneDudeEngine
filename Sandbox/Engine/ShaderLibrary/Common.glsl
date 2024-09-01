#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

float Square(float v){
	return v * v;
}

vec3 SafeNormalize(vec3 v){
    return normalize(v);
}

vec3 saturate(vec3 v){
    return clamp(v, 0.0, 1.0);
}

float saturate(float v){
    return clamp(v, 0.0, 1.0);
}

float Pow4(float x){
    return (x * x) * (x * x);
}

#endif