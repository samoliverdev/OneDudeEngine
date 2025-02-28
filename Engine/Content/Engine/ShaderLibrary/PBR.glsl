#ifndef PBR_INCLUDED
#define PBR_INCLUDED

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;

/*uniform vec3 _AmbientLight;
uniform samplerCube _IrradianceMap;
uniform samplerCube _PrefilterMap; 
uniform float _SkyLightIntensity;
uniform sampler2D _BrdfLUT;*/

float DistributionGGX(vec3 N, vec3 H, float roughness){
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness){
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0){
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness){
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

vec3 IncomingLight(Surface surface, Light light){
	vec3 albedo = surface.color;
	float metallic = surface.metallic;
	float roughness = surface.smoothness;
	float ao = surface.occlusion;

	vec3 N = surface.normal; //normalize(Normal); 
    vec3 V = surface.viewDirection; //normalize(camPos - WorldPos);
	vec3 L = light.direction; //normalize(lightPositions[i] - WorldPos);
    vec3 H = normalize(V + L);

	vec3 radiance = light.color * light.attenuation;
	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, albedo, metallic);
	vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0); 

	float NDF = DistributionGGX(N, H, roughness);       
	float G = GeometrySmith(N, V, L, roughness);  

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0)  + 0.0001;
	vec3 specular = numerator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	
	kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);        
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

float PerceptualRoughnessToMipmapLevel(float perceptualRoughness){
    perceptualRoughness = perceptualRoughness * (1.7 - 0.7 * perceptualRoughness);
    return perceptualRoughness * MAX_REFLECTION_LOD;
}

vec3 AmbientLight(Surface surfaceWS){
    //return _AmbientLight * surfaceWS.occlusion;

	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, surfaceWS.color, surfaceWS.metallic);
	vec3 R = reflect(-surfaceWS.viewDirection, surfaceWS.normal); 
    vec3 F = fresnelSchlickRoughness(max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0), F0, surfaceWS.smoothness);
    vec3 kS = F;
    vec3 kD = vec3(1.0 - kS);
    kD *= 1.0 - surfaceWS.metallic;	  
    vec3 irradiance = _AmbientLight + SampleTextureCube(_IrradianceMap, _IrradianceMapSampler, surfaceWS.normal).rgb * _SkyLightIntensity;
    vec3 diffuse = irradiance * surfaceWS.color;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    
	//float mip = PerceptualRoughnessToMipmapLevel(surfaceWS.smoothness); //surfaceWS.smoothness * MAX_REFLECTION_LOD
	float mip = surfaceWS.smoothness * MAX_REFLECTION_LOD;
    //vec3 prefilteredColor = _AmbientLight + textureLod(_PrefilterMap, R, mip).rgb * _SkyLightIntensity;    
    vec3 prefilteredColor = _AmbientLight + SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, R, mip).rgb * _SkyLightIntensity;   
    vec2 brdf = SampleTexture2D(_BrdfLUT, _BrdfLUTSampler, vec2(max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0), surfaceWS.smoothness)).rg;
    vec3 specular = /*_AmbientLight +*/ (prefilteredColor * (F * brdf.x + brdf.y));

    return (kD * diffuse + specular) * surfaceWS.occlusion;
}

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

	//color = color / (color + vec3(1.0));
	//color = pow(color, vec3(1.0/2.2)); 

	return color;
}

#endif