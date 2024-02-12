#ifndef SURFACE_INCLUDED
#define SURFACE_INCLUDED

struct Surface{
	vec3 position;
	vec3 normal;
    vec3 viewDirection;
	float depth;
	vec3 color;
	float alpha;
	float occlusion;
    float metallic;
	float smoothness;
};

#endif