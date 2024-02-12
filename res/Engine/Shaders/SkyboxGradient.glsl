#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;

out vec3 pos;

uniform mat4 view;
uniform mat4 projection;

void main() {
    pos = _pos;
    gl_Position = projection * view * vec4(pos, 1.0);
}
#endif

#if defined(FRAGMENT)

in vec3 pos;

out vec4 fragColor;

vec4 color1 = vec4(0.4, 0.96, 1.0, 0.92);
vec4 color2 = vec4(0.0, 0.24, 1.0, 1.0);

float Remap(float In, vec2 InMinMax, vec2 OutMinMax){
    return OutMinMax.x + (In - InMinMax.x) * (OutMinMax.y - OutMinMax.x) / (InMinMax.y - InMinMax.x);
}

void main() {
    float t = Remap(normalize(pos).y, vec2(-1,1), vec2(0, 1));
    fragColor = mix(color1, color2, t);
}
#endif