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

void main() {
    const float gamma = 2.2;
    vec3 color = texture(mainTex, texCoord).rgb;
    color.rgb = min(color.rgb, 60.0);
    color /= (color + vec3(1.0));
    
    fragColor = vec4(color, 1.0);
}
#endif