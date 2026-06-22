//SOURCE: https://docs.unity3d.com/Packages/com.unity.shadergraph@17.3/manual/Simple-Noise-Node.html
#ifndef SIMPLE_NOISE_INCLUDED
#define SIMPLE_NOISE_INCLUDED
float noise_randomValue(vec2 uv){
    return fract(sin(dot(uv, vec2(12.9898, 78.233)))*43758.5453);
}

float Hash21(vec2 p){
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float noise_interpolate(float a, float b, float t){
    return (1.0-t)*a + (t*b);
}

float valueNoise(vec2 uv){
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    f = f * f * (3.0 - 2.0 * f);

    uv = abs(fract(uv) - 0.5);
    vec2 c0 = i + vec2(0.0, 0.0);
    vec2 c1 = i + vec2(1.0, 0.0);
    vec2 c2 = i + vec2(0.0, 1.0);
    vec2 c3 = i + vec2(1.0, 1.0);
    float r0 = Hash21(c0); //noise_randomValue(c0);
    float r1 = Hash21(c1); //noise_randomValue(c1);
    float r2 = Hash21(c2); //noise_randomValue(c2);
    float r3 = Hash21(c3); //noise_randomValue(c3);

    float bottomOfGrid = noise_interpolate(r0, r1, f.x);
    float topOfGrid = noise_interpolate(r2, r3, f.x);
    float t = noise_interpolate(bottomOfGrid, topOfGrid, f.y);
    return t;
}

float SimpleNoise(vec2 UV, float Scale){
    float t = 0.0;

    float freq = pow(2.0, float(0));
    float amp = pow(0.5, float(3-0));
    t += valueNoise(vec2(UV.x*Scale/freq, UV.y*Scale/freq))*amp;

    freq = pow(2.0, float(1));
    amp = pow(0.5, float(3-1));
    t += valueNoise(vec2(UV.x*Scale/freq, UV.y*Scale/freq))*amp;

    freq = pow(2.0, float(2));
    amp = pow(0.5, float(3-2));
    t += valueNoise(vec2(UV.x*Scale/freq, UV.y*Scale/freq))*amp;

    return t;
}

#endif