#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;
layout (location = 2) in vec3 _normal;

layout (location = 10) in mat4 _modelInstancing;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 pos;
out vec3 normal;
out vec2 texCoord;

uniform float useInstancing = 0;

void main(){
    pos = _pos;
    normal = _normal;
    texCoord = _texCoord;

    gl_Position = projection * view * (useInstancing >= 1.0 ? _modelInstancing : model) * vec4(pos, 1.0);
}
#endif

#if defined(FRAGMENT)
uniform sampler2D mainTex;
uniform vec4 color = vec4(1,1,1,1);

in vec3 pos;
in vec3 normal;
in vec2 texCoord;

out vec4 fragColor;

#include TestLib.glsl

void main(){
    vec4 texColor = texture(mainTex, texCoord);
    if(texColor.a < 0.1) discard;

//#define TEST

#if defined(TEST)
    fragColor = texColor * GetColor();
#else
    fragColor = texColor * color;
#endif

}
#endif