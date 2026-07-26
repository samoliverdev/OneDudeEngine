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
	float roughness = surface.roughness;
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
    vec3 F = fresnelSchlickRoughness(max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0), F0, surfaceWS.roughness);
    vec3 kS = F;
    vec3 kD = vec3(1.0 - kS);
    kD *= 1.0 - surfaceWS.metallic;	  
    vec3 irradiance = _AmbientLight.rgb + SampleTextureCube(_IrradianceMap, _IrradianceMapSampler, surfaceWS.normal).rgb * _SkyLightIntensity;
    vec3 diffuse = irradiance * surfaceWS.color;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    
	//float mip = PerceptualRoughnessToMipmapLevel(surfaceWS.roughness); //surfaceWS.smoothness * MAX_REFLECTION_LOD
	float mip = surfaceWS.roughness * MAX_REFLECTION_LOD;
    //vec3 prefilteredColor = _AmbientLight + textureLod(_PrefilterMap, R, mip).rgb * _SkyLightIntensity;    
    vec3 prefilteredColor = _AmbientLight.rgb + SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, R, mip).rgb * _SkyLightIntensity;   
    vec2 brdf = SampleTexture2D(_BrdfLUT, _BrdfLUTSampler, vec2(max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0), surfaceWS.roughness)).rg;
    vec3 specular = /*_AmbientLight +*/ (prefilteredColor * (F * brdf.x + brdf.y));

    // === Fresnel Occlusion / Specular Visibility ===
    float NdotV = max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0);
    float visibility = clamp(NdotV + surfaceWS.occlusion, 0.0, 1.0);
    specular *= visibility;
    // ===============================================

    return (kD * diffuse + specular) * surfaceWS.occlusion;
}

vec3 IncomingLight2(Surface surface, Light light) {
    vec3 N = surface.normal; 
    vec3 V = surface.viewDirection; 
    vec3 L = light.direction; 
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    vec3 radiance = light.color * light.attenuation;

    // 1. CLEAR COAT LAYER (Direct)
    // Clear coat typically uses a fixed F0 of 0.04 (standard polyurethane/lacquer IOR of ~1.5)
    vec3 F0_coat = vec3(0.04);
    vec3 F_coat = fresnelSchlick(HdotV, F0_coat) * surface.clearCoat; 
    
    float NDF_coat = DistributionGGX(N, H, surface.clearCoatRoughness);
    float G_coat   = GeometrySmith(N, V, L, surface.clearCoatRoughness);
    
    vec3 specularCoat = (NDF_coat * G_coat * F_coat) / (4.0 * NdotV * NdotL + 0.0001);

    // 2. BASE LAYER (Direct)
    vec3 F0_base = mix(vec3(0.04), surface.color, surface.metallic);
    vec3 F_base  = fresnelSchlick(HdotV, F0_base); 

    float NDF_base = DistributionGGX(N, H, surface.roughness);       
    float G_base   = GeometrySmith(N, V, L, surface.roughness);  

    vec3 specularBase = (NDF_base * G_base * F_base) / (4.0 * NdotV * NdotL + 0.0001);
    vec3 kD = (vec3(1.0) - F_base) * (1.0 - surface.metallic);
    vec3 diffuseBase = kD * surface.color / PI;

    // 3. ENERGY MODULATION / BLENDING
    // Clear coat absorbs energy before it reaches the base layer
    vec3 baseLayerColor = (diffuseBase + specularBase) * (vec3(1.0) - F_coat);
    
    return (baseLayerColor + specularCoat) * radiance * NdotL;
}

vec3 AmbientLight2(Surface surfaceWS) {
    vec3 N = surfaceWS.normal;
    vec3 V = surfaceWS.viewDirection;
    vec3 R = reflect(-V, N); 
    float NdotV = max(dot(N, V), 0.0);
    
    // 1. CLEAR COAT INDIRECT (IBL)
    vec3 F0_coat = vec3(0.04);
    vec3 F_coat = fresnelSchlickRoughness(NdotV, F0_coat, surfaceWS.clearCoatRoughness) * surfaceWS.clearCoat;
    
    float mipCoat = surfaceWS.clearCoatRoughness * MAX_REFLECTION_LOD;
    vec3 prefilteredCoat = _AmbientLight.rgb + SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, R, mipCoat).rgb * _SkyLightIntensity;
    vec2 brdfCoat = SampleTexture2D(_BrdfLUT, _BrdfLUTSampler, vec2(NdotV, surfaceWS.clearCoatRoughness)).rg;
    vec3 specularCoat = prefilteredCoat * (F_coat * brdfCoat.x + brdfCoat.y);
    
    // Clear coat visibility mask
    float coatVisibility = clamp(NdotV + surfaceWS.occlusion, 0.0, 1.0);
    specularCoat *= coatVisibility;

    // 2. BASE INDIRECT (IBL)
    vec3 F0_base = mix(vec3(0.04), surfaceWS.color, surfaceWS.metallic);
    vec3 F_base = fresnelSchlickRoughness(NdotV, F0_base, surfaceWS.roughness);
    
    vec3 kD = (vec3(1.0) - F_base) * (1.0 - surfaceWS.metallic);   
    
    vec3 cubeIrradiance = SampleTextureCube(_IrradianceMap, _IrradianceMapSampler, N).rgb;
    vec3 diffuseBase = (_AmbientLight.rgb + cubeIrradiance * _SkyLightIntensity) * surfaceWS.color;
    
    float mipBase = surfaceWS.roughness * MAX_REFLECTION_LOD;
    vec3 prefilteredBase = _AmbientLight.rgb + SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, R, mipBase).rgb * _SkyLightIntensity;   
    vec2 brdfBase = SampleTexture2D(_BrdfLUT, _BrdfLUTSampler, vec2(NdotV, surfaceWS.roughness)).rg;
    vec3 specularBase = prefilteredBase * (F_base * brdfBase.x + brdfBase.y);
    
    float baseVisibility = clamp(NdotV + surfaceWS.occlusion, 0.0, 1.0);
    specularBase *= baseVisibility;

    // 3. COMPOSITION
    vec3 baseLayerAmbient = (kD * diffuseBase + specularBase) * (vec3(1.0) - F_coat);
    
    return (baseLayerAmbient + specularCoat) * surfaceWS.occlusion;
}

float IORToF0(float ior){
    float f = (ior - 1.0) / (ior + 1.0);
    return f * f;
}

vec3 ClearCoatFresnel(float cosTheta, float ior){
    float F0 = IORToF0(ior);
    return fresnelSchlick(cosTheta, vec3(F0));
}


vec3 ClearCoatBRDF(Surface surface, vec3 L){
    vec3 N = surface.normal;
    vec3 V = surface.viewDirection;

    vec3 H = normalize(V + L);

    float NdotV = max(dot(N,V),0.0);
    float NdotL = max(dot(N,L),0.0);
    float VdotH = max(dot(V,H),0.0);

    float roughness = surface.clearCoatRoughness;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = ClearCoatFresnel(VdotH, surface.clearCoatIOR);

    vec3 numerator = NDF * G * F;

    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    return numerator / denominator;
}

vec3 IncomingLight3(Surface surface, Light light){
    vec3 albedo = surface.color;
    float metallic = surface.metallic;
    float roughness = surface.roughness;

    vec3 N = surface.normal;
    vec3 V = surface.viewDirection;
    vec3 L = light.direction;
    vec3 H = normalize(V + L);

    vec3 radiance = light.color * light.attenuation;

    //Base PBR
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H,V),0.0), F0);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) +0.0001;

    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 base = kD * albedo / PI + specular;

    //Clear Coat
    vec3 coat = ClearCoatBRDF(surface, L);

    //Energy loss through coat
    base *= 1.0 - surface.clearCoat * 0.25;
    
    float NdotL = max(dot(N,L),0.0);

    return (base + coat * surface.clearCoat) * radiance * NdotL;
}

vec3 AmbientLight3(Surface surfaceWS){
    vec3 N = surfaceWS.normal;
    vec3 V = surfaceWS.viewDirection;

    //Base Fresnel
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, surfaceWS.color, surfaceWS.metallic);

    vec3 F = fresnelSchlickRoughness(max(dot(N,V),0.0), F0, surfaceWS.roughness);
    vec3 kS = F;
    vec3 kD = vec3(1.0)-kS;

    kD *= 1.0 - surfaceWS.metallic;

    //Diffuse IBL
    vec3 irradiance = _AmbientLight.rgb + SampleTextureCube(_IrradianceMap, _IrradianceMapSampler, N).rgb * _SkyLightIntensity;

    vec3 diffuse = irradiance * surfaceWS.color;

    //Base specular IBL
    vec3 R = reflect(-V, N);

    float mip = surfaceWS.roughness * MAX_REFLECTION_LOD;
    vec3 prefilteredColor = _AmbientLight.rgb + SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, R, mip).rgb * _SkyLightIntensity;

    vec2 brdf =SampleTexture2D(_BrdfLUT, _BrdfLUTSampler, vec2(max(dot(N,V),0.0), surfaceWS.roughness)).rg;

    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    //Clear Coat IBL
    vec3 coatR = reflect(-V, N);
    float coatMip = surfaceWS.clearCoatRoughness * MAX_REFLECTION_LOD;
    vec3 coatPrefilter = SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, coatR, coatMip).rgb * _SkyLightIntensity;
    vec3 coatF = ClearCoatFresnel(max(dot(N,V),0.0), surfaceWS.clearCoatIOR);
    vec3 clearCoatSpec = coatPrefilter * coatF * surfaceWS.clearCoat;

    //Fresnel visibility
    float NdotV = max(dot(N,V),0.0);
    float visibility = clamp(NdotV + surfaceWS.occlusion, 0.0, 1.0);
    specular *= visibility;


    //Final composition
    vec3 result = kD * diffuse + specular;
    result *= 1.0 - surfaceWS.clearCoat * 0.25;
    result += clearCoatSpec;
    return result * surfaceWS.occlusion;
}

vec3 GetFinalColor(Surface surfaceWS){
    ShadowData shadowData = GetShadowData(surfaceWS);
	
	vec3 color = AmbientLight(surfaceWS);// * surfaceWS.color * surfaceWS.occlusion;

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