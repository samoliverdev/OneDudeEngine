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

#include res/Engine/ShaderLibrary/Core.glsl

uniform sampler2D mainTex;
uniform float option;

in vec3 pos;
in vec2 texCoord;
out vec4 fragColor;

void main() {
    fragColor = texture(mainTex, texCoord);
    fragColor.rgb = ApplyGamaCorrection(fragColor.rgb);
}
#endif