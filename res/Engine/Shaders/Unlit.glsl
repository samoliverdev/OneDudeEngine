#version 330 core

#pragma SupportInstancing true
#pragma MultiCompile _ SKINNED INSTANCING

#if defined(VERTEX)

/*
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;
layout (location = 2) in vec3 _normal;
layout (location = 10) in mat4 _modelInstancing;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float useInstancing = 0;
*/

#include res/Engine/ShaderLibrary/Vertex.glsl

out vec3 _pos;
out vec3 _normal;
out vec2 _texCoord;

void main(){
    //mat4 targetModelMatrix = (useInstancing >= 1.0 ? modelInstancing : model);
    mat4 targetModelMatrix = GetModelMatrix();

    _pos = pos;
    _normal = normal;
    _texCoord = texCoord;

    //gl_Position = projection * view * targetModelMatrix * vec4(pos, 1.0);
    gl_Position = projection * view * targetModelMatrix * GetLocalPos();
}
#endif

#if defined(FRAGMENT)
uniform sampler2D mainTex;
uniform vec4 color = vec4(1,1,1,1);

in vec3 _pos;
in vec3 _normal;
in vec2 _texCoord;

out vec4 fragColor;

#include res/Engine/Shaders/TestLib.glsl

void main(){
    vec4 texColor = texture(mainTex, _texCoord);
    if(texColor.a < 0.1) discard;

//#define TEST

#if defined(TEST)
    fragColor = texColor * GetColor();
#else
    fragColor = texColor * color;
#endif

}
#endif