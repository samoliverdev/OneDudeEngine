#ifndef BASE_SHADER_INPUTS_INCLUDED
#define BASE_SHADER_INPUTS_INCLUDED

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

struct ShaderData{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
};

#endif