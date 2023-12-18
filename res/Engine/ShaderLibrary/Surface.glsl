#ifndef SURFACE_INCLUDED
#define SURFACE_INCLUDED

struct Surface{
	vec3 normal;
    vec3 viewDirection;
	vec3 color;
	float alpha;
    float metallic;
	float smoothness;
};

#endif