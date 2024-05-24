#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;
out vec2 texCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    texCoord = _texCoord;
    gl_Position = projection * view * model * vec4(_pos, 1.0);
}
#endif

#if defined(FRAGMENT)
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D mainTex;
uniform vec4 color;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(mainTex, texCoord).r);
    fragColor = vec4(color.rgb, 1.0) * sampled;
}
#endif