#ifndef SHADOWS_INCLUDED
#define SHADOWS_INCLUDED

#define _RECEIVE_SHADOWS

#define _DIRECTIONAL_PCF
#define DIRECTIONAL_FILTER_SAMPLES 2

#define MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT 4
#define MAX_SHADOWED_OTHER_LIGHT_COUNT 16
#define MAX_CASCADE_COUNT 4

uniform sampler2DArray _DirectionalShadowAtlas;
uniform mat4 _DirectionalShadowMatrices[MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT * MAX_CASCADE_COUNT];
uniform int _CascadeCount;
uniform float _CascadeCullingSpheres[MAX_CASCADE_COUNT];
uniform float _ShadowDistance;
uniform vec4 _ShadowAtlasSize;
uniform vec4 _ShadowDistanceFade;

uniform sampler2DArray _OtherShadowAtlas;
uniform mat4 _OtherShadowMatrices[MAX_SHADOWED_OTHER_LIGHT_COUNT];

float _ShadowBias = 0.001;

struct DirectionalShadowData{
	float strength;
	int tileIndex;
    float diffuseFactor;
};

struct OtherShadowData{
    float strength;
    int tileIndex;
    bool isPoint;
    float diffuseFactor;
    int shadowMaskChannel;
    vec3 lightPositionWS;
	vec3 lightDirectionWS;
	//vec3 spotDirectionWS;
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

        if(i == _CascadeCount && _CascadeCount > 0) data.strength = 0.0;
    }


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
    ///*
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
    //*/
    
    /*
    vec2 texelSize = 1.0 / textureSize(_DirectionalShadowAtlas, 0).xy;
    float shadow;
    float swidth = 0.6;
    float endp = swidth * 3.0 + swidth / 2.0;
    for (float y = -endp; y <= endp; y += swidth) {
        for (float x = -endp; x <= endp; x += swidth) {
            shadow += SampleDirectionalShadowAtlas(
                positionSTS + vec4(x * texelSize.x, y * texelSize.y, 0, 0),
                layer,
                diffuseFactor
            );
        }
    }
    return shadow / 64;
    */
#else
    return SampleDirectionalShadowAtlas(positionSTS, layer, diffuseFactor);
#endif
}

float SampleOtherShadowAltas(vec4 positionSTS, int layer, float diffuseFactor){
    vec3 projCoords = positionSTS.xyz / positionSTS.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(_OtherShadowAtlas, vec3(projCoords.xy, layer)).r; 
    
    float currentDepth = projCoords.z;
    if(currentDepth > 1.0) return 1.0;
    
    float bias = _ShadowBias/4;
    //bias = 0.001/2;
    bias = mix(bias, 0.0, diffuseFactor);

    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 

    return 1 - shadow;
}

float FilterOtherShadow(vec4 positionSTS, int layer, float diffuseFactor){
#if defined(_DIRECTIONAL_PCF)
    ///*
    float shadow = 0;
    vec2 texelSize = 1.0 / textureSize(_OtherShadowAtlas, 0).xy;
    int sampleRadius = DIRECTIONAL_FILTER_SAMPLES;
    for(int x = -sampleRadius; x <= sampleRadius; x++){
        for(int y = -sampleRadius; y <= sampleRadius; y++){
            shadow += SampleOtherShadowAltas(
                positionSTS + (vec4(x, y, 0, 0) * vec4(texelSize.xy, 1,1)) ,
                layer,
                diffuseFactor
            );
        }
    }
    shadow /= pow((sampleRadius * 2 + 1), 2);
    return shadow;
    //*/

    /*
    vec2 texelSize = 1.0 / textureSize(_OtherShadowAtlas, 0).xy;
    float shadow;
    float swidth = 0.6;
    float endp = swidth * 3.0 + swidth / 2.0;
    for (float y = -endp; y <= endp; y += swidth) {
        for (float x = -endp; x <= endp; x += swidth) {
            shadow += SampleOtherShadowAltas(
                positionSTS + vec4(x * texelSize.x, y * texelSize.y, 0, 0),
                layer,
                diffuseFactor
            );
        }
    }
    return shadow / 64;
    */
#else
    return SampleOtherShadowAltas(positionSTS, layer, diffuseFactor);
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

#define CUBEMAPFACE_POSITIVE_X 0
#define CUBEMAPFACE_NEGATIVE_X 1
#define CUBEMAPFACE_POSITIVE_Y 2
#define CUBEMAPFACE_NEGATIVE_Y 3
#define CUBEMAPFACE_POSITIVE_Z 4
#define CUBEMAPFACE_NEGATIVE_Z 5

int CubeMapFaceID(vec3 dir){
    int faceID;

    if(abs(dir.z) >= abs(dir.x) && abs(dir.z) >= abs(dir.y)){
        faceID = (dir.z < 0.0) ? CUBEMAPFACE_NEGATIVE_Z : CUBEMAPFACE_POSITIVE_Z;
    } else if(abs(dir.y) >= abs(dir.x)){
        faceID = (dir.y < 0.0) ? CUBEMAPFACE_NEGATIVE_Y : CUBEMAPFACE_POSITIVE_Y;
    } else {
        faceID = (dir.x < 0.0) ? CUBEMAPFACE_NEGATIVE_X : CUBEMAPFACE_POSITIVE_X;
    }

    return faceID;
}

float GetOtherShadow(OtherShadowData other, ShadowData global, Surface surfaceWS){
    int tileIndex = other.tileIndex;
    if(other.isPoint){
        int faceOffset = CubeMapFaceID(-other.lightDirectionWS);
        tileIndex += faceOffset;
    }
    
    vec3 normalBias = surfaceWS.normal * 0.0; //surfaceWS.interpolatedNormal * 0.0;
    vec4 positionSTS = _OtherShadowMatrices[tileIndex] * vec4(surfaceWS.position + normalBias, 1.0);

    return FilterOtherShadow(positionSTS, tileIndex, other.diffuseFactor);
}

float GetOtherShadowAttenuation(OtherShadowData other, ShadowData global, Surface surfaceWS){
    #if !defined(_RECEIVE_SHADOWS)
        return 1.0;
    #endif
    
    float shadow = GetOtherShadow(other, global, surfaceWS);
    
    return shadow;
}

#endif