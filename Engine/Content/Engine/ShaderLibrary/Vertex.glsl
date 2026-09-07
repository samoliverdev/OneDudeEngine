#ifndef VERTEX_INCLUDED
#define VERTEX_INCLUDED

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
    Uniform mat4 invProjection;
    Uniform mat4 invView;
EndUniform()

#if defined(OpenGL_API) && defined(UseUniformBuffer) && !defined(OpenGL_API_New)
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

#if defined(SKINNED) || defined(SKINNED2)
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

#ifdef INSTANCINGMATRIX43
    layout(location = 10) in vec4 a_ModelMatrix_0;
    layout(location = 11) in vec4 a_ModelMatrix_1;
    layout(location = 12) in vec4 a_ModelMatrix_2;
#endif

const int MAX_BONES = 120;
const int MAX_BONE_INFLUENCE = 4;

#if defined(SKINNED)
    uniform mat4 animated[MAX_BONES];
#endif

#if defined(SKINNED2)
layout(std140) uniform PerDrawData {
    mat4 animated[MAX_BONES];
};
#endif

/*
#ifdef SKINNED
#undef INSTANCING
#endif
*/

vec4 GetPerInstanceData(){
    #ifdef INSTANCING
        return vec4(modelInstancing[0][3], modelInstancing[1][3], modelInstancing[2][3], modelInstancing[3][3]);
    #else 
        return vec4(model[0][3], model[1][3], model[2][3], model[3][3]);
    #endif
}

mat4 GetModelMatrix(){
#ifdef INSTANCING
    #ifdef OpenGL_API
        mat4 result = modelInstancing;
        result[0][3] = 0;
        result[1][3] = 0;
        result[2][3] = 0;
        result[3][3] = 1;
        return result;
        //return modelInstancing;
    #else
    mat4 result = mat4(a_ModelMatrix_0, a_ModelMatrix_1, a_ModelMatrix_2, a_ModelMatrix_3);;
        result[0][3] = 0;
        result[1][3] = 0;
        result[2][3] = 0;
        result[3][3] = 1;
        return result;
    //return mat4(a_ModelMatrix_0, a_ModelMatrix_1, a_ModelMatrix_2, a_ModelMatrix_3);
    #endif
#elif defined(INSTANCINGMATRIX43)
    //INFO: This can be bug, becose probaly a_ModelMatrix_0 is not row, i think is colum
    return transpose(mat4(a_ModelMatrix_0, a_ModelMatrix_1, a_ModelMatrix_2, vec4(0,0,0,1)));
    /*return mat4(
        vec4(a_ModelMatrix_0.x, a_ModelMatrix_1.x, a_ModelMatrix_2.x, 0.0), // col 0
        vec4(a_ModelMatrix_0.y, a_ModelMatrix_1.y, a_ModelMatrix_2.y, 0.0), // col 1
        vec4(a_ModelMatrix_0.z, a_ModelMatrix_1.z, a_ModelMatrix_2.z, 0.0), // col 2
        vec4(a_ModelMatrix_0.w, a_ModelMatrix_1.w, a_ModelMatrix_2.w, 1.0)  // col 3 (translation / w)
    );*/
#else
    /*#ifdef SKINNED2
        mat4 scaleMat = mat4(1.0);
        scaleMat[0][0] = 5;
        scaleMat[1][1] = 5;
        scaleMat[2][2] = 5;
        return model * scaleMat; // or scaleMat * model (see below)
    #else*/
        mat4 result = model;
        result[0][3] = 0;
        result[1][3] = 0;
        result[2][3] = 0;
        result[3][3] = 1;
        return result;
        //return model;
    //#endif
#endif
}

vec4 GetLocalPos(){
#if defined(SKINNED) || defined(SKINNED2)
    mat4 skin = animated[boneIds.x] * weights.x +
    animated[boneIds.y] * weights.y +
    animated[boneIds.z] * weights.z +
    animated[boneIds.w] * weights.w;
    return skin * vec4(pos, 1.0);
#else
    return vec4(pos, 1.0);
#endif
}

vec3 GetLocalNormal(){
#if defined(SKINNED) || defined(SKINNED2)
    mat3 skinNormalMatrix = mat3(animated[boneIds.x]) * weights.x +
                        mat3(animated[boneIds.y]) * weights.y +
                        mat3(animated[boneIds.z]) * weights.z +
                        mat3(animated[boneIds.w]) * weights.w;
    return normalize(skinNormalMatrix * normal);
#else
    return normalize(normal);
#endif
}

vec3 GetLocalTangent(){
#if defined(SKINNED) || defined(SKINNED2)
    mat3 skinTangentMatrix = mat3(animated[boneIds.x]) * weights.x +
                             mat3(animated[boneIds.y]) * weights.y +
                             mat3(animated[boneIds.z]) * weights.z +
                             mat3(animated[boneIds.w]) * weights.w;

    return normalize(skinTangentMatrix * tangents);
#else
    return normalize(tangents);
#endif
}

#endif

#endif