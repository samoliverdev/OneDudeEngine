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
	float roughness;
	float clearCoat;
	float clearCoatRoughness;
	float clearCoatIOR;          // 1.0 - 3.0
};

#endif