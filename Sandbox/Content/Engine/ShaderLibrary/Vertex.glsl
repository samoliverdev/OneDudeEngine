#ifndef VERTEX_INCLUDED
#define VERTEX_INCLUDED

#if defined(VERTEX)

layout(location = 0) in vec3 pos;
#ifdef UV3
layout(location = 1) in vec3 texCoord;
#else
layout(location = 1) in vec2 texCoord;
#endif
layout(location = 2) in vec3 normal;
layout(location = 4) in vec3 tangents;

#ifdef SKINNED
layout(location = 5) in ivec4 boneIds;
layout(location = 6) in vec4 weights;
#endif

#ifdef INSTANCING
    #ifdef OpenGL_API
    layout(location = 10) in mat4 modelInstancing;
    #else   
    layout(location = 5) in vec4 a_ModelMatrix_0;
    layout(location = 6) in vec4 a_ModelMatrix_1;
    layout(location = 7) in vec4 a_ModelMatrix_2;
    layout(location = 8) in vec4 a_ModelMatrix_3;
    #endif
#endif

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
EndUniform()

BeginUniform(1, 0, PerDraw)
    Uniform mat4 model;
EndUniform()

#ifdef SKINNED
const int MAX_BONES = 120;
const int MAX_BONE_INFLUENCE = 4;
BeginUniform(1, 1, Anim)
    Uniform mat4 animated[MAX_BONES];
EndUniform()
#endif

/*
#ifdef SKINNED
#undef INSTANCING
#endif
*/

mat4 GetModelMatrix(){
#ifdef INSTANCING
    #ifdef OpenGL_API
    return modelInstancing;
    #else
    return mat4(a_ModelMatrix_0, a_ModelMatrix_1, a_ModelMatrix_2, a_ModelMatrix_3);
    #endif
#else
    return model;
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

#endif