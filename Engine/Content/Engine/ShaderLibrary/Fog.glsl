#ifndef FOG_INCLUDED
#define FOG_INCLUDED

const float a = 0.001;
const float b = 0.001;

vec4 ApplyFog( 
    in vec4  col, // color of pixel
    in float t  // distance to point
){
    float fogAmount = 1.0 - exp(-t * b);
    vec4 fogColor = vec4(0.5, 0.6, 0.7, 1);
    return mix(col, fogColor, fogAmount);
}

#endif