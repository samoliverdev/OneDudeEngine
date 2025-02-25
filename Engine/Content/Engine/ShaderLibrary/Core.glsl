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

vec4 ToSRGB(vec4 a){
#if defined(ENABLE_GAMA_CORRECTION)
    return vec4(pow(a.r, gamma), pow(a.g, gamma), pow(a.b, gamma), a.a);
#else
    return a;
#endif
}

#if defined(OpenGL_API)
    #define SampleTexture2D(tex, sample, uv) texture(tex, uv)

    vec4 textureSRGB(sampler2D tex, vec2 uv){
    #if defined(ENABLE_GAMA_CORRECTION)
        vec4 color = texture(tex, uv); //return pow(color, vec4(gamma));
        color.rgb = pow(color.rgb, vec3(gamma));
        return color;
    #else
        return texture(tex, uv);
    #endif 
    }

    vec4 textureSRGB(samplerCube tex, vec3 uv){
    #if defined(ENABLE_GAMA_CORRECTION)
        vec4 color = texture(tex, uv); //return pow(color, vec4(gamma));
        color.rgb = pow(color.rgb, vec3(gamma));
        return color;
    #else
        return texture(tex, uv);
    #endif 
    }
#endif

#if defined(WebGPU_API)
    #define SampleTexture2D(tex, sample, uv) texture(sampler2D(tex, sample), uv)

    vec4 textureSRGB(texture2D tex, sampler s, vec2 uv){
    #if defined(ENABLE_GAMA_CORRECTION)
        vec4 color = texture(sampler2D(tex, s), uv); //return pow(color, vec4(gamma));
        color.rgb = pow(color.rgb, vec3(gamma));
        return color;
    #else
        return texture(sampler2D(tex, s), uv);
    #endif 
    }
#endif

#endif