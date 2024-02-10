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

//Source: https://github.com/dmnsgn/glsl-tone-map/blob/main/aces.glsl
// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x){
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float aces(float x){
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    const float gamma = 2.2;
    vec3 color = texture(mainTex, texCoord).rgb;
    color.rgb = min(color.rgb, 60.0);
    color = aces(color);
    
    fragColor = vec4(color, 1.0);
}
#endif