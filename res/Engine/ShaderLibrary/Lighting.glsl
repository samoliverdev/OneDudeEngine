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

vec3 GetLightingFinal2(Surface surfaceWS, BRDF brdf){
	vec3 color = vec3(0.0);
	for(int i = 0; i < GetDirectionalLightCount(); i++){
		ShadowData shadowData = GetShadowData(surfaceWS, i);
		//ShadowData shadowData = GetShadowData(surfaceWS, GetDirectionalLightBaseTileIndex(i));
		Light light = GetDirectionalLight(i, surfaceWS, shadowData);

		color += GetLighting(surfaceWS, brdf, light);
		//color += GetLighting(surfaceWS, brdf, GetDirectionalLight(i, surfaceWS));
	}
	return color;
}

#endif