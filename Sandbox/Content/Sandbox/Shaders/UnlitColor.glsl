#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;
layout (location = 2) in vec3 _normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 pos;
out vec3 normal;
out vec2 texCoord;

void main() {
    pos = _pos;
    normal = _normal;
    texCoord = _texCoord;

    gl_Position = projection * view * model * vec4(pos, 1.0);
}
#endif

#if defined(FRAGMENT)
uniform vec3 color;

in vec3 pos;
in vec3 normal;
in vec2 texCoord;

out vec4 fragColor;

void main() {
    fragColor = vec4(color, 1);
}
#endif