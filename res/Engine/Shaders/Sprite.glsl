#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 _texCoord;

void main() {
    _texCoord = texCoord;

    gl_Position = projection * view * model * vec4(pos, 1.0);
}
#endif

#if defined(FRAGMENT)
uniform sampler2D texture1;
in vec2 _texCoord;
out vec4 fragColor;

void main() {
    fragColor = texture(texture1, _texCoord);
    if(fragColor.a < 0.1) discard;
}
#endif