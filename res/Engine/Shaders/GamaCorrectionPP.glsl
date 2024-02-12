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
uniform float option;

in vec3 pos;
in vec2 texCoord;

out vec4 fragColor;

const float offset = 1.0 / 300.0; 

void main() {
    fragColor = texture(mainTex, texCoord);

    float gamma = 2.2;
    fragColor.rgb = pow(fragColor.rgb, vec3(1.0/gamma));
}
#endif