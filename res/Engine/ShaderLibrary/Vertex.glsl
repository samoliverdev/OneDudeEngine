#ifndef VERTEX_INCLUDED
#define VERTEX_INCLUDED

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
layout (location = 5) in ivec4 boneIds;
layout (location = 6) in vec4 weights;
layout (location = 10) in mat4 modelInstancing;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float useInstancing = 0;
uniform float isSkinned = 0;

const int MAX_BONES = 120;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 animated[MAX_BONES];

/*
#ifdef SKINNED
#undef INSTANCING
#endif
*/

mat4 GetModelMatrix(){
#ifdef INSTANCING
    return modelInstancing;
#else
    return model ;
#endif
}

vec4 GetLocalPos(){
#ifdef SKINNED
    mat4 skin = animated[boneIds.x] * weights.x +
    animated[boneIds.y] * weights.y +
    animated[boneIds.z] * weights.z +
    animated[boneIds.w] * weights.w;
    return skin * vec4(pos, 1.0);
#else
    return vec4(pos, 1.0);
#endif
}

#endif