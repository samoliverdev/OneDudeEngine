#ifndef LIGHT_INCLUDED
#define LIGHT_INCLUDED

#define MAX_DIRECTIONAL_LIGHT_COUNT 4
uniform int _DirectionalLightCount;
uniform vec4 _DirectionalLightColors[MAX_DIRECTIONAL_LIGHT_COUNT];
uniform vec4 _DirectionalLightDirections[MAX_DIRECTIONAL_LIGHT_COUNT];
uniform vec4 _DirectionalLightShadowData[MAX_DIRECTIONAL_LIGHT_COUNT];

#define MAX_OTHER_LIGHT_COUNT 16
uniform int _OtherLightCount;
uniform vec4 _OtherLightColors[MAX_OTHER_LIGHT_COUNT];
uniform vec4 _OtherLightPositions[MAX_OTHER_LIGHT_COUNT];
uniform vec4 _OtherLightDirections[MAX_OTHER_LIGHT_COUNT];
uniform vec4 _OtherLightSpotAngles[MAX_OTHER_LIGHT_COUNT];
uniform vec4 _OtherLightShadowData[MAX_OTHER_LIGHT_COUNT];

struct Light{
	vec3 color;
	vec3 direction;
	float attenuation;
};

DirectionalShadowData GetDirectionalShadowData(int lightIndex, ShadowData shadowData){
	DirectionalShadowData data;
	data.strength = _DirectionalLightShadowData[lightIndex].x * shadowData.strength;
	data.tileIndex = int(_DirectionalLightShadowData[lightIndex].y) + shadowData.cascadeIndex;
	data.diffuseFactor = 0;
	return data;
}

DirectionalShadowData GetDirectionalShadowData(int lightIndex, ShadowData shadowData, Surface surfaceWS){
	DirectionalShadowData data;
	data.strength = _DirectionalLightShadowData[lightIndex].x * shadowData.strength;
	data.tileIndex = int(_DirectionalLightShadowData[lightIndex].y) + shadowData.cascadeIndex;
	data.diffuseFactor = dot(surfaceWS.normal, -_DirectionalLightDirections[lightIndex].xyz);
	return data;
}

OtherShadowData GetOtherShadowData(int lightIndex){
	OtherShadowData data;
	data.strength = _OtherLightShadowData[lightIndex].x;
	data.tileIndex = int(_OtherLightShadowData[lightIndex].y);
	data.isPoint = _OtherLightShadowData[lightIndex].z == 1.0;
	data.lightDirectionWS = vec3(0);
	data.lightPositionWS = vec3(0);
	//data.diffuseFactor = dot(surfaceWS.normal, -_DirectionalLightDirections[lightIndex].xyz);
	data.shadowMaskChannel = int(_OtherLightShadowData[lightIndex].w);
	return data;
}

int GetDirectionalLightCount(){
	return _DirectionalLightCount;
}

Light GetDirectionalLight(int index, Surface surfaceWS, ShadowData shadowData){
	Light light;
	light.color = _DirectionalLightColors[index].rgb;
	light.direction = _DirectionalLightDirections[index].xyz;
	DirectionalShadowData dirShadowData = GetDirectionalShadowData(index, shadowData, surfaceWS);
	light.attenuation = GetDirectionalShadowAttenuation(dirShadowData, surfaceWS);
	return light;
}

int GetOtherLightCount(){
	return _OtherLightCount;
}

Light GetOtherLight(int index, Surface surfaceWS, ShadowData shadowData){
	Light light;
	light.color = _OtherLightColors[index].rgb;
	vec3 ray = _OtherLightPositions[index].xyz - surfaceWS.position;
	light.direction = normalize(ray);
	float distanceSqr = max(dot(ray, ray), 0.00001);
	float rangeAttenuation = Square(
		saturate(1.0 - Square(distanceSqr * _OtherLightPositions[index].w))
	);
	//float rangeAttenuation = saturate(1.0 / (length(ray) * _OtherLightPositions[index].w));
	
	vec4 spotAngles = _OtherLightSpotAngles[index];
	float spotAttenuation = Square(
		saturate(dot(_OtherLightDirections[index].xyz, light.direction) * 
		spotAngles.x + spotAngles.y)
	);
	
	OtherShadowData otherShadowData = GetOtherShadowData(index);
	otherShadowData.lightDirectionWS = light.direction;
	otherShadowData.lightPositionWS = _OtherLightPositions[index].xyz;

	light.attenuation = GetOtherShadowAttenuation(otherShadowData, shadowData, surfaceWS) *
		spotAttenuation * rangeAttenuation / distanceSqr;
	
	return light;
}

#endif