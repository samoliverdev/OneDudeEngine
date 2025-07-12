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

//#define Test_CODE

BRDF GetBRDF(Surface surface){
	#ifndef Test_CODE
	BRDF brdf;
    float oneMinusReflectivity = OneMinusReflectivity(surface.metallic);
    brdf.diffuse = surface.color * oneMinusReflectivity;
	brdf.specular = mix(vec3(MIN_REFLECTIVITY), surface.color, surface.metallic);
    brdf.perceptualRoughness = PerceptualSmoothnessToPerceptualRoughness(surface.smoothness);
    brdf.roughness = max(PerceptualRoughnessToRoughness(brdf.perceptualRoughness), 0.02);
	brdf.fresnel = saturate(surface.smoothness + 1.0 - oneMinusReflectivity);
    return brdf;
	#else

	/*
	BRDF brdf;

    float metallic = clamp(surface.metallic, 0.0, 1.0);
    float smoothness = clamp(surface.smoothness, 0.0, 1.0);

    float perceptualRoughness = 1.0 - smoothness;
    float roughness = max(perceptualRoughness * perceptualRoughness, 0.02);

    vec3 dielectricSpecular = vec3(MIN_REFLECTIVITY);
    vec3 F0 = mix(dielectricSpecular, surface.color, metallic);

    // ⚠️ Unity-style energy compensation (base reflectivity já consome parte da luz)
    float oneMinusReflectivity = 1.0 - max(max(F0.r, F0.g), F0.b); // mesmo que no seu OneMinusReflectivity()
    brdf.diffuse = surface.color * oneMinusReflectivity;

    brdf.specular = F0;
    brdf.perceptualRoughness = perceptualRoughness;
    brdf.roughness = roughness;

    return brdf;
	*/

	BRDF brdf;

    float metallic = clamp(surface.metallic, 0.0, 1.0);
    float smoothness = clamp(surface.smoothness, 0.0, 1.0);

    float perceptualRoughness = 1.0 - smoothness;
    float roughness = max(perceptualRoughness * perceptualRoughness, 0.02);

    vec3 dielectricF0 = vec3(MIN_REFLECTIVITY);
    vec3 F0 = mix(dielectricF0, surface.color, metallic); // F0 = albedo se metal

    brdf.specular = F0;
    brdf.diffuse = surface.color * (1.0 - metallic); // ⚠️ Essa é a chave!
    brdf.perceptualRoughness = perceptualRoughness;
    brdf.roughness = roughness;

    return brdf;
	#endif
}

float SpecularStrength(Surface surface, BRDF brdf, Light light){
	vec3 h = SafeNormalize(light.direction + surface.viewDirection);
	float nh2 = Square(saturate(dot(surface.normal, h)));
	float lh2 = Square(saturate(dot(light.direction, h)));
	float r2 = Square(brdf.roughness);
	float d2 = Square(nh2 * (r2 - 1.0) + 1.00001);
	#ifndef Test_CODE
	float normalization = brdf.roughness * 4.0 + 2.0;
	#else
	float normalization = (brdf.roughness + 1.0) * (brdf.roughness + 1.0);
	#endif
	return r2 / (d2 * (lh2 + 0.001) * normalization);
}

vec3 DirectBRDF(Surface surface, BRDF brdf, Light light){
	return SpecularStrength(surface, brdf, light) * brdf.specular + brdf.diffuse;
}
 
vec3 _fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness){
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

vec3 IndirectBRDF(Surface surface, BRDF brdf, vec3 diffuse, vec3 specular){
	#ifndef Test_CODE

	float fresnelStrength = surface.smoothness * Pow4(1.0 - saturate(dot(surface.normal, surface.viewDirection)));
	vec3 reflection = specular * mix(brdf.specular, vec3(brdf.fresnel), fresnelStrength);
	reflection /= brdf.roughness * brdf.roughness + 1.0;
    return diffuse * brdf.diffuse + reflection;

	#else

	float cosTheta = saturate(dot(surface.normal, surface.viewDirection));

    // Fresnel com roughness (usado para difusa compensada apenas)
    vec3 F = _fresnelSchlickRoughness(cosTheta, brdf.specular, brdf.roughness);
    vec3 kD = (1.0 - F) * (1.0 - surface.metallic); // difusa só se não for metálico

    vec3 _diffuse = diffuse * brdf.diffuse * kD;
    vec3 _specular = specular; // já ponderado por LUT e F0 no SampleEnvironmentSpecular

    return (_diffuse + _specular) * surface.occlusion;
	#endif
}

#endif