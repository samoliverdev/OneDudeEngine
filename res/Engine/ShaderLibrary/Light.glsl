#ifndef LIGHT_INCLUDED
#define LIGHT_INCLUDED

#define MAX_DIRECTIONAL_LIGHT_COUNT 4
uniform int _DirectionalLightCount;
uniform vec4 _DirectionalLightColors[MAX_DIRECTIONAL_LIGHT_COUNT];
uniform vec4 _DirectionalLightDirections[MAX_DIRECTIONAL_LIGHT_COUNT];
uniform vec4 _DirectionalLightShadowData[MAX_DIRECTIONAL_LIGHT_COUNT];

struct Light{
	vec3 color;
	vec3 direction;
	float attenuation;
};

DirectionalShadowData GetDirectionalShadowData(int lightIndex, ShadowData shadowData){
	DirectionalShadowData data;
	data.strength = _DirectionalLightShadowData[lightIndex].x * shadowData.strength;
	data.tileIndex = int(_DirectionalLightShadowData[lightIndex].y) + shadowData.cascadeIndex;
	return data;
}

int GetDirectionalLightCount(){
	return _DirectionalLightCount;
}

/*Light GetDirectionalLight(int index){
	Light light;
	light.color = _DirectionalLightColors[index].rgb;
	light.direction = _DirectionalLightDirections[index].xyz;
	return light;
}*/

Light GetDirectionalLight(int index, Surface surfaceWS, ShadowData shadowData){
	Light light;
	light.color = _DirectionalLightColors[index].rgb;
	light.direction = _DirectionalLightDirections[index].xyz;
	DirectionalShadowData dirShadowData = GetDirectionalShadowData(index, shadowData);
	light.attenuation = GetDirectionalShadowAttenuation(dirShadowData, surfaceWS);
	//light.attenuation = shadowData.cascadeIndex * 0.25;
	return light;
}

#endif