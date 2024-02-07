#ifndef SHADOWS_INCLUDED
#define SHADOWS_INCLUDED

#define _DIRECTIONAL_PCF
#define DIRECTIONAL_FILTER_SAMPLES 2

#define MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT 4
#define MAX_CASCADE_COUNT 4

uniform sampler2DArray _DirectionalShadowAtlas;
uniform mat4 _DirectionalShadowMatrices[MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT * MAX_CASCADE_COUNT];
uniform int _CascadeCount;
uniform float _CascadeCullingSpheres[MAX_CASCADE_COUNT];
uniform float _ShadowDistance;
uniform vec4 _ShadowAtlasSize;
uniform vec4 _ShadowDistanceFade;

float _ShadowBias = 0.001;

struct DirectionalShadowData{
	float strength;
	int tileIndex;
    float diffuseFactor;
};

struct ShadowData{
	int cascadeIndex;
    float strength;
};

float FadedShadowStrength(float distance, float scale, float fade){
	return saturate((1.0 - distance * scale) * fade);
}

ShadowData GetShadowData(Surface surfaceWS){
    vec4 fragPosViewSpace = view * vec4(surfaceWS.position, 1);
    float depthValue = abs(fragPosViewSpace.z);

	ShadowData data;
	data.cascadeIndex = 0;
    data.strength = 1.0;
    //data.strength = surfaceWS.depth < _ShadowDistance ? 1.0 : 0.0;
    data.strength = FadedShadowStrength(
		surfaceWS.depth, _ShadowDistanceFade.x, _ShadowDistanceFade.y
	);

    for(int i = 0; i < _CascadeCount; i++){
        if(depthValue <= _CascadeCullingSpheres[i]){
            data.cascadeIndex = i;
            break;
        }
    }

    //if(i == _CascadeCount) data.strength = 0.0;

	return data;
}

/*
float SampleDirectionalShadowAtlas(vec4 positionSTS){
    vec3 projCoords = positionSTS.xyz / positionSTS.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(_DirectionalShadowAtlas, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    if(currentDepth > 1.0) return 1.0;

    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0; 

    return 1 - shadow;
}
*/

float SampleDirectionalShadowAtlas(vec4 positionSTS, int layer, float diffuseFactor){
    vec3 projCoords = positionSTS.xyz / positionSTS.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(_DirectionalShadowAtlas, vec3(projCoords.xy, layer)).r; 
    
    float currentDepth = projCoords.z;
    if(currentDepth > 1.0) return 1.0;
    
    float bias = _ShadowBias/4;
    //bias = 0.001/2;

    //float diffuseFactor = dot(normal, -lightDir);
    bias = mix(bias, 0.0, diffuseFactor);

    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 

    return 1 - shadow;
}

float FilterDirectionalShadow(vec4 positionSTS, int layer, float diffuseFactor){
#if defined(_DIRECTIONAL_PCF)
    float shadow = 0;
    vec2 texelSize = 1.0 / textureSize(_DirectionalShadowAtlas, 0).xy;
    int sampleRadius = DIRECTIONAL_FILTER_SAMPLES;
    for(int x = -sampleRadius; x <= sampleRadius; x++){
        for(int y = -sampleRadius; y <= sampleRadius; y++){
            shadow += SampleDirectionalShadowAtlas(
                positionSTS + (vec4(x, y, 0, 0) * vec4(texelSize.xy, 1,1)) ,
                layer,
                diffuseFactor
            );
        }
    }
    shadow /= pow((sampleRadius * 2 + 1), 2);
    return shadow;
#else
    return SampleDirectionalShadowAtlas(positionSTS, layer, diffuseFactor);
#endif
}

float GetDirectionalShadowAttenuation(DirectionalShadowData data, Surface surfaceWS){
    if(data.strength <= 0.0) return 1.0;

    //vec2 texelSize = 1.0 / textureSize(_DirectionalShadowAtlas, 0).xy;
    //vec3 normalBias = surfaceWS.normal * vec3(texelSize.xy, 1);

	vec4 positionSTS = _DirectionalShadowMatrices[data.tileIndex] * vec4(surfaceWS.position /*+ normalBias*/, 1.0);
    //vec4 positionSTS = _DirectionalShadowMatrix * vec4(surfaceWS.position, 1.0);
	//float shadow = SampleDirectionalShadowAtlas(positionSTS, data.tileIndex);
    float shadow = FilterDirectionalShadow(positionSTS, data.tileIndex, data.diffuseFactor);
	
    return mix(1.0, shadow, data.strength);
}

#endif