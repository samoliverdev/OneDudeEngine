#ifndef CORE_INCLUDED
#define CORE_INCLUDED

#define ENABLE_GAMA_CORRECTION
//#undef ENABLE_GAMA_CORRECTION

const float gamma = 2.2;

vec3 ApplyGamaCorrection(vec3 rgb){
#if defined(ENABLE_GAMA_CORRECTION)
    return pow(rgb, vec3(1.0/gamma));
#else
    return rgb;
#endif
}

vec4 ToLinear(vec4 a){
#if defined(ENABLE_GAMA_CORRECTION)
    return vec4(pow(a.rgb, vec3(gamma)), a.a); // Convert sRGB to Linear
#else
    return a;
#endif
}

vec3 ToLinear(vec3 a){
#if defined(ENABLE_GAMA_CORRECTION)
    return pow(a.rgb, vec3(gamma)); // Convert sRGB to Linear
#else
    return a;
#endif
}

#if defined(OpenGL_API)
    #define SampleTexture2D(tex, sample, uv) texture(tex, uv)
    #define SampleTexture2DArray(tex, sample, uv) texture(tex, uv)
    #define SampleTextureCube(tex, sample, uv) texture(tex, uv)
    #define SampleTextureCubeLod(tex, sample, uv, lod) textureLod(tex, uv, lod)
#endif

#if defined(WebGPU_API)
    #define SampleTexture2D(tex, sample, uv) texture(sampler2D(tex, sample), uv)
    #define SampleTexture2DArray(tex, sample, uv) vec4(0)
    #define SampleTextureCube(tex, sample, uv) vec4(0)
    #define SampleTextureCubeLod(tex, sample, uv, lod) vec4(0)
#endif

#endif