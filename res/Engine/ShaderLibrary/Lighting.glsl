#ifndef LIGHTING_INCLUDED
#define LIGHTING_INCLUDED

vec3 IncomingLight(Surface surface, Light light){
	return light.color * ( saturate(dot(surface.normal, light.direction)) * light.attenuation );
}

vec3 GetLighting(Surface surface, BRDF brdf, Light light){
	return IncomingLight(surface, light) * DirectBRDF(surface, brdf, light);
}

/*vec3 GetLightingFinal(Surface surface, BRDF brdf){
    vec3 color = vec3(0.0);
	for(int i = 0; i < GetDirectionalLightCount(); i++){
		color += GetLighting(surface, brdf, GetDirectionalLight(i));
	}
	return color;
}*/

vec3 GetLightingFinal2(Surface surfaceWS, BRDF brdf, GI gi){
	//vec3 color = vec3(0.0);
	ShadowData shadowData = GetShadowData(surfaceWS);
	vec3 color = gi.diffuse * brdf.diffuse;
	for(int i = 0; i < GetDirectionalLightCount(); i++){
		Light light = GetDirectionalLight(i, surfaceWS, shadowData);
		color += GetLighting(surfaceWS, brdf, light);
	}
	return color;
}

#endif