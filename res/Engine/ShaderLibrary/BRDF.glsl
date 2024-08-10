#ifndef BRDF_INCLUDED
#define BRDF_INCLUDED

struct BRDF{
	vec3 diffuse;
	vec3 specular;
	float roughness;
	float perceptualRoughness;
	float fresnel;
};

#define MIN_REFLECTIVITY 0.04

float OneMinusReflectivity(float metallic){
	float range = 1.0 - MIN_REFLECTIVITY;
	return range - metallic * range;
}

float PerceptualRoughnessToRoughness(float perceptualRoughness){
    return perceptualRoughness * perceptualRoughness;
}

float PerceptualSmoothnessToPerceptualRoughness(float perceptualSmoothness){
    return (1.0 - perceptualSmoothness);
}

BRDF GetBRDF(Surface surface){
	BRDF brdf;

    float oneMinusReflectivity = OneMinusReflectivity(surface.metallic);
    brdf.diffuse = surface.color * oneMinusReflectivity;
	brdf.specular = mix(vec3(MIN_REFLECTIVITY), surface.color, surface.metallic);
    brdf.perceptualRoughness = PerceptualSmoothnessToPerceptualRoughness(surface.smoothness);
    brdf.roughness = PerceptualRoughnessToRoughness(brdf.perceptualRoughness);
	brdf.fresnel = saturate(surface.smoothness + 1.0 - oneMinusReflectivity);
	
    return brdf;
}

float SpecularStrength(Surface surface, BRDF brdf, Light light){
	vec3 h = SafeNormalize(light.direction + surface.viewDirection);
	float nh2 = Square(saturate(dot(surface.normal, h)));
	float lh2 = Square(saturate(dot(light.direction, h)));
	float r2 = Square(brdf.roughness);
	float d2 = Square(nh2 * (r2 - 1.0) + 1.00001);
	float normalization = brdf.roughness * 4.0 + 2.0;
	return r2 / (d2 * max(0.1, lh2) * normalization);
}

vec3 DirectBRDF(Surface surface, BRDF brdf, Light light){
	return SpecularStrength(surface, brdf, light) * brdf.specular + brdf.diffuse;
}

vec3 IndirectBRDF(Surface surface, BRDF brdf, vec3 diffuse, vec3 specular){
	//return diffuse * brdf.diffuse;

	float fresnelStrength = surface.smoothness * Pow4(1.0 - saturate(dot(surface.normal, surface.viewDirection))); //surface.fresnelStrength
	vec3 reflection = specular * mix(brdf.specular, vec3(brdf.fresnel), fresnelStrength);
	//reflection = specular * brdf.specular;
	reflection /= brdf.roughness * brdf.roughness + 1.0;
	
    return diffuse * brdf.diffuse + reflection;
	
}

#endif