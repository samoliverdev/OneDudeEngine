#ifndef BASE_LIGHTING_INCLUDED
#define BASE_LIGHTING_INCLUDED

vec3 AmbientLight(Surface surfaceWS);
vec3 IncomingLight(Surface surface, Light light);

vec3 GetFinalColor(Surface surfaceWS){
    ShadowData shadowData = GetShadowData(surfaceWS);
	
	vec3 color = AmbientLight(surfaceWS);
	for(int i = 0; i < GetDirectionalLightCount(); i++){
		Light light = GetDirectionalLight(i, surfaceWS, shadowData);
		color += IncomingLight(surfaceWS, light);
	}
	for(int j = 0; j < GetOtherLightCount(); j++){
		Light light = GetOtherLight(j, surfaceWS, shadowData);
		color += IncomingLight(surfaceWS, light);
	}

	return color;
}

#endif