#ifndef VERTEX_INCLUDED
#define VERTEX_INCLUDED

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
EndUniform()

#if defined(OpenGL_API) && defined(UseUniformBuffer)
    uniform mat4 model;
#else
    BeginUniform(1, 0, PerDraw)
        Uniform mat4 model;
    EndUniform()
#endif

#if defined(VERTEX)

layout(location = 0) in vec3 pos;
#ifdef UV3
layout(location = 1) in vec3 texCoord;
#else
layout(location = 1) in vec2 texCoord;
#endif
layout(location = 2) in vec3 normal;
layout(location = 4) in vec3 tangents;


#ifdef OpenGL_API
layout(location = 10) in mat4 modelInstancing;
#else   
layout(location = 5) in vec4 a_ModelMatrix_0;
layout(location = 6) in vec4 a_ModelMatrix_1;
layout(location = 7) in vec4 a_ModelMatrix_2;
layout(location = 8) in vec4 a_ModelMatrix_3;
#endif

vec4 GetColor(){
    return modelInstancing[3];
}

mat4 GetModelMatrix(){
    return transpose(mat4(
        modelInstancing[0], 
        modelInstancing[1], 
        modelInstancing[2], 
        vec4(0,0,0,1)
    ));
}

vec4 GetLocalPos(){
    return vec4(pos, 1.0);
}

vec3 GetLocalNormal(){
    return normalize(normal);
}

vec3 GetLocalTangent(){
    return normalize(tangents);
}

#endif

#endif