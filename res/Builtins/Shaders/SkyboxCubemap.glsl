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
uniform samplerCube mainTex;

in vec3 pos;

out vec4 fragColor;

void main() {
    fragColor = texture(mainTex, pos);
}
#endif