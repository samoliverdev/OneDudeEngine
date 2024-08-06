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

vec4 textureSRGB(sampler2D tex, vec2 uv){
#if defined(ENABLE_GAMA_CORRECTION)
    vec4 color = texture(tex, uv);
    //return pow(color, vec4(gamma));
    color.rgb = pow(color.rgb, vec3(gamma));
    return color;
#else
    return texture(tex, uv);
#endif 
}

#endif