#ifndef BASE_INCLUDED
#define BASE_INCLUDED

#if defined(OpenGL_API)
    precision highp float;
    precision highp int;

    #define In(loc) in
    #define InFlat(loc) flat in
    #define Out(loc) out
    #define OutFlat(loc) flat out
    #define Attribute(loc) layout(location = loc) in

    #define OutPosition gl_Position
    #define VertexIndex gl_VertexID

    #if defined(UseUniformBuffer)
        #define BeginUniform(inSet, inBinding, name) layout(std140) uniform name{
        #define BeginCameraUniform() layout(std140) uniform CamDraw{
        #define BeginMaterialUniform() layout(std140) uniform Main{
        #define BeginPerDrawUniform() //layout(std140) uniform PerDraw{
        #define EndUniform() };
        #define Uniform
    #else
        #define BeginUniform(inSet, inBinding, name)

        #define EndUniform()
        #define Uniform uniform
    #endif

    #define Texture2D(inset, inbinding, name, nameSampler) uniform sampler2D name;
    #define Texture2DArray(inset, inbinding, name, nameSampler) uniform sampler2DArray name;
    #define TextureCube(inset, inbinding, name, nameSampler) uniform samplerCube name;

    #define TextureSize(tex, lod) textureSize(tex, lod)

#endif

#if defined(WebGPU_API)
    #define In(loc) layout(location = loc) in 
    #define InFlat(loc) layout(location = loc) flat in
    #define Out(loc) layout(location = loc) out 
    #define OutFlat(loc) layout(location = loc) flat out
    #define Attribute(loc) layout(location = loc) in

    #define OutPosition gl_Position
    #define VertexIndex gl_VertexIndex

    #define BeginUniform(inSet, inBinding, name) layout(set = inSet, binding = inBinding) uniform name {
    #define EndUniform() };
    #define Uniform

    #define Texture2D(inset, inbinding, name, nameSampler) layout(set = inset, binding = (inbinding * 2 - 2 + 1)) uniform texture2D name; layout(set = inset, binding = (inbinding * 2 - 2 + 1) + 1) uniform sampler nameSampler;
    #define Texture2DArray(inset, inbinding, name, nameSampler) layout(set = inset, binding = (inbinding * 2 - 2 + 1)) uniform texture2D name; layout(set = inset, binding = (inbinding * 2 - 2 + 1) + 1) uniform sampler nameSampler;
    #define TextureCube(inset, inbinding, name, nameSampler) layout(set = inset, binding = (inbinding * 2 - 2 + 1)) uniform texture2D name;  layout(set = inset, binding = (inbinding * 2 - 2 + 1) + 1) uniform sampler nameSampler;

    #define TextureSize(tex, lod) vec3(1)
    
#endif

#endif