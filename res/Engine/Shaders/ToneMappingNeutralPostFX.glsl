#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;

out vec3 pos;
out vec2 texCoord;

void main() {
    pos = _pos;
    texCoord = _texCoord;
    gl_Position = vec4(pos, 1.0);
}
#endif

#if defined(FRAGMENT)
uniform sampler2D mainTex;

in vec3 pos;
in vec2 texCoord;

out vec4 fragColor;

//
// Neutral tonemapping (Hable/Hejl/Frostbite)
// Input is linear RGB
//
vec3 NeutralCurve(vec3 x, float a, float b, float c, float d, float e, float f){
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

float NeutralCurve(float x, float a, float b, float c, float d, float e, float f){
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

vec3 NeutralTonemap(vec3 x){
    // Tonemap
    float a = 0.2;
    float b = 0.29;
    float c = 0.24;
    float d = 0.272;
    float e = 0.02;
    float f = 0.3;
    float whiteLevel = 5.3;
    float whiteClip = 1.0;

    vec3 whiteScale = vec3(1.0) / NeutralCurve(whiteLevel, a, b, c, d, e, f);
    x = NeutralCurve(x * whiteScale, a, b, c, d, e, f);
    x *= whiteScale;

    // Post-curve white point adjustment
    x /= vec3(whiteClip);

    return x;
}

void main() {
    //const float gamma = 2.2;
    vec3 color = texture(mainTex, texCoord).rgb;
    color.rgb = min(color.rgb, 60.0);
    color = NeutralTonemap(color);
    
    fragColor = vec4(color, 1.0);
}
#endif