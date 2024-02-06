#ifndef SHADOWS_INCLUDED
#define SHADOWS_INCLUDED

#define MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT 4
#define MAX_CASCADE_COUNT 4

uniform sampler2DArray _DirectionalShadowAtlas;
uniform mat4 _DirectionalShadowMatrices[MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT * MAX_CASCADE_COUNT];
uniform int _CascadeCount;
uniform float _CascadeCullingSpheres[MAX_CASCADE_COUNT];
uniform float _ShadowDistance;
uniform vec4 _ShadowDistanceFade;

uniform float _ShadowBias = 0.001;

struct DirectionalShadowData{
	float strength;
	int tileIndex;
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

float SampleDirectionalShadowAtlas(vec4 positionSTS, int layer){
    vec3 projCoords = positionSTS.xyz / positionSTS.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(_DirectionalShadowAtlas, vec3(projCoords.xy, layer)).r; 
    
    float currentDepth = projCoords.z;
    if(currentDepth > 1.0) return 1.0;
    
    float bias = _ShadowBias;

    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 

    return 1 - shadow;
}

float GetDirectionalShadowAttenuation(DirectionalShadowData data, Surface surfaceWS){
    if(data.strength <= 0.0) return 1.0;

	vec4 positionSTS = _DirectionalShadowMatrices[data.tileIndex] * vec4(surfaceWS.position, 1.0);
    //vec4 positionSTS = _DirectionalShadowMatrix * vec4(surfaceWS.position, 1.0);
	float shadow = SampleDirectionalShadowAtlas(positionSTS, data.tileIndex);
	
    return mix(1.0, shadow, data.strength);
}

#endif