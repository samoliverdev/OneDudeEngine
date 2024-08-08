#ifndef LIGHTING_INCLUDED
#define LIGHTING_INCLUDED

vec3 IncomingLight(Surface surface, Light light){
	return saturate(dot(surface.normal, light.direction) * light.attenuation) * light.color;
}

vec3 GetLighting(Surface surface, BRDF brdf, Light light){
	return IncomingLight(surface, light) * DirectBRDF(surface, brdf, light);
}

vec3 GetLighting(Surface surfaceWS, BRDF brdf, GI gi){
	ShadowData shadowData = GetShadowData(surfaceWS);

	vec3 color = IndirectBRDF(surfaceWS, brdf, gi.diffuse, gi.specular);
	for(int i = 0; i < GetDirectionalLightCount(); i++){
		Light light = GetDirectionalLight(i, surfaceWS, shadowData);
		color += GetLighting(surfaceWS, brdf, light);
	}
	for(int j = 0; j < GetOtherLightCount(); j++){
		Light light = GetOtherLight(j, surfaceWS, shadowData);
		color += GetLighting(surfaceWS, brdf, light);
	}

	return color;
}

#endif