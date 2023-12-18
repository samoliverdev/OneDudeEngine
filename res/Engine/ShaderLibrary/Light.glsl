#ifndef LIGHT_INCLUDED
#define LIGHT_INCLUDED

#define MAX_DIRECTIONAL_LIGHT_COUNT 4
uniform int _DirectionalLightCount;
uniform vec4 _DirectionalLightColors[MAX_DIRECTIONAL_LIGHT_COUNT];
uniform vec4 _DirectionalLightDirections[MAX_DIRECTIONAL_LIGHT_COUNT];

struct Light{
	vec3 color;
	vec3 direction;
};

int GetDirectionalLightCount(){
	return _DirectionalLightCount;
}

Light GetDirectionalLight(int index){
	Light light;
	light.color = _DirectionalLightColors[index].rgb;
	light.direction = _DirectionalLightDirections[index].xyz;
	return light;
}

#endif