#ifndef PBR_INCLUDED
#define PBR_INCLUDED

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;

/*uniform vec3 _AmbientLight;
uniform samplerCube _IrradianceMap;
uniform samplerCube _PrefilterMap; 
uniform float _SkyLightIntensity;
uniform sampler2D _BrdfLUT;*/

const float clearCoatIOR = 1.5;

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

////////////////////////////////////

float IORToF0(float ior){
    float f = (ior - 1.0) / (ior + 1.0);
    return f * f;
}

vec3 ClearCoatFresnel(float cosTheta, float ior){
    float F0 = IORToF0(ior);
    return fresnelSchlick(cosTheta, vec3(F0));
}

#define DIFFUSE_BRDF_LAMBERT
//#define DIFFUSE_BRDF_BURLEY
//#define DIFFUSE_BRDF_OREN_NAYAR

#define SPECULAR_BRDF_GGX
//#define SPECULAR_BRDF_GGX_CUSTOM

////////////////// Lambert //////////////////////
vec3 DiffuseBRDF_Lambert(vec3 albedo){
    return albedo / PI;
}

vec3 DiffuseIBL_Lambert(vec3 irradiance, vec3 albedo){
    // Irradiance is already the integral of
    // Lambert BRDF over the hemisphere.
    return irradiance * albedo;
}

//////////////////// Burley ////////////////////////
vec3 DiffuseBRDF_Burley(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness){
    vec3 H = normalize(V + L);
    
    float LdotH = max(dot(L, H), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float Fd90 = 0.5 + 2.0 * LdotH * LdotH * roughness;
    float FL = pow(clamp(1.0 - NdotL, 0.0, 1.0), 5.0);
    float FV = pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);

    // Disney Diffuse formula: (1 + (Fd90 - 1)*FL) * (1 + (Fd90 - 1)*FV) * (albedo / PI)
    float scatter = (1.0 + (Fd90 - 1.0) * FL) * (1.0 + (Fd90 - 1.0) * FV);
    return (albedo / PI) * scatter;
}

////////////////// Burley (Disney) IBL //////////////////////
// Modulates irradiance based on NdotV and surface roughness 
// to approximate retro-reflection under ambient light.
vec3 DiffuseIBL_Burley(vec3 irradiance, vec3 albedo, float NdotV, float roughness){
    float Fd90 = 0.5 + 2.0 * roughness; // Simplified retro-reflection factor at grazing angles
    float FV = pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
    float scatter = mix(1.0, Fd90, FV); // Modulates ambient response based on view angle
    
    return irradiance * albedo * scatter;
}

///////////////////// OrenNayar //////////////////////
vec3 DiffuseBRDF_OrenNayar(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness){
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // Roughness parameter conversion (variance sigma^2)
    float sigma2 = roughness * roughness;

    // Standard analytical approximation constants A and B
    float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
    float B = 0.45 * (sigma2 / (sigma2 + 0.09));

    // Calculate max(0, cos(phi_i - phi_r)) using projected vector dot product
    vec3 lightProj = normalize(L - N * NdotL);
    vec3 viewProj  = normalize(V - N * NdotV);
    float cosPhiDiff = max(0.0, dot(lightProj, viewProj));

    // Angle sine & tangent approximations via dot products
    float sinAlpha, tanBeta;
    if (NdotL < NdotV) {
        sinAlpha = sqrt(1.0 - NdotL * NdotL);
        tanBeta  = sqrt(1.0 - NdotV * NdotV) / max(NdotV, 0.0001);
    } else {
        sinAlpha = sqrt(1.0 - NdotV * NdotV);
        tanBeta  = sqrt(1.0 - NdotL * NdotL) / max(NdotL, 0.0001);
    }

    return (albedo / PI) * (A + B * cosPhiDiff * sinAlpha * tanBeta);
}

////////////////// Oren-Nayar IBL //////////////////////
// Modulates irradiance using the roughness variance parameter
// to simulate ambient darkening and diffuse flattening.
vec3 DiffuseIBL_OrenNayar(vec3 irradiance, vec3 albedo, float NdotV, float roughness){
    float sigma2 = roughness * roughness;
    float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
    float B = 0.45 * (sigma2 / (sigma2 + 0.09));
    
    // 1. Clamp NdotV above zero to prevent division by near-zero at extreme grazing angles
    float safeNdotV = max(NdotV, 0.08); 

    // 2. Compute sin and tan with safe bounds
    float sinThetaV = sqrt(max(0.0, 1.0 - safeNdotV * safeNdotV));
    
    // Clamp tanThetaV to avoid sky-high values that blow out tone mappers
    float tanThetaV = min(sinThetaV / safeNdotV, 2.5);
    
    // 3. Smooth fade at extreme edges using (1 - NdotV)^5 fade factor
    float edgeFade = 1.0 - pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
    
    // Ambient modifier with edge dampening
    float ambientModifier = A + (B * 0.5 * sinThetaV * tanThetaV) * edgeFade;
    
    // Clamp total factor to a reasonable range [0.0, 1.5]
    ambientModifier = clamp(ambientModifier, 0.0, 1.5);
    
    return irradiance * albedo * ambientModifier;
}

////////////////// GGX //////////////////////
vec3 SpecularBRDF_GGX(vec3 N, vec3 V, vec3 L, vec3 F0, float roughness){
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    vec3 F = fresnelSchlick(HdotV, F0);

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    return (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);
}

vec3 SpecularIBL_GGX(vec3 specular, vec3 F, float NdotV, float roughness){
    vec2 brdf = SampleTexture2D(_BrdfLUT, _BrdfLUTSampler, vec2(NdotV, roughness)).rg;
    return specular * (F * brdf.x + brdf.y);
}

vec3 ClearCoatBRDF_GGX(Surface surface, vec3 L){
    vec3 N = surface.normal;
    vec3 V = surface.viewDirection;

    vec3 H = normalize(V + L);

    float NdotV = max(dot(N,V),0.0);
    float NdotL = max(dot(N,L),0.0);
    float VdotH = max(dot(V,H),0.0);

    float roughness = surface.clearCoatRoughness;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = ClearCoatFresnel(VdotH, clearCoatIOR);

    vec3 numerator = NDF * G * F;

    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    return numerator / denominator;
}

//////////////////////////////////////////////
////////////////// Custom Callisto / Skin Dual-Lobe GGX //////////////////////
vec3 SpecularBRDF_SkinGGX(vec3 N, vec3 V, vec3 L, vec3 F0, float roughness) {
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0001);
    float HdotV = max(dot(H, V), 0.0);

    // 1. Dual Lobe Roughness Definitions
    // Primary Lobe: Catches the pore detail and tight highlights (Moisture)
    float roughnessPrimary = clamp(roughness * 0.55, 0.02, 1.0);
    // Secondary Lobe: Spreads light broadly across skin contours
    float roughnessSecondary = clamp(roughness * 1.45, 0.02, 1.0);

    // 2. Normal Distribution Functions (NDF)
    float D_Primary   = DistributionGGX(N, H, roughnessPrimary);
    float D_Secondary = DistributionGGX(N, H, roughnessSecondary);

    // Blend lobes (80% broad base, 20% sharp peak gives realistic wet skin)
    float D_Skin = mix(D_Secondary, D_Primary, 0.25);

    // 3. Modified Geometry Factor (Smoother visibility shadow)
    float G = GeometrySmith(N, V, L, roughness);

    // 4. Custom Fresnel (Boosts subtle reflections on micro-surface angles)
    vec3 F = fresnelSchlick(HdotV, F0);

    // Composite Dual-Lobe Specular BRDF
    vec3 specular = (D_Skin * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    
    return specular;
}

vec3 SpecularIBL_SkinGGX(vec3 specular, vec3 F, float NdotV, float roughness){
    // Dual-lobe mipmap blending for environment specular
    float mipPrimary   = (roughness * 0.55) * MAX_REFLECTION_LOD;
    float mipSecondary = (roughness * 1.45) * MAX_REFLECTION_LOD;
    
    vec2 brdf = SampleTexture2D(_BrdfLUT, _BrdfLUTSampler, vec2(NdotV, roughness)).rg;
    
    // Gives indirect skin highlights the same sharp-yet-soft depth seen on the right
    return specular * (F * brdf.x + brdf.y);
}


///////////////////////////////////////////////

struct GI{
    vec3 diffuseLighting;
    vec3 specularLighting;
    vec3 clearCoatSpecularLighting;
};

GI GetGI(Surface surface){
    GI gi;

    vec3 N = normalize(surface.normal); //surface.normal;
    vec3 V = normalize(surface.viewDirection); //surface.viewDirection;
    vec3 R = reflect(-V, N);

    vec3 F0 = mix(vec3(0.04), surface.color, surface.metallic);
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, surface.roughness);

    // Common Irradiance sampling
    vec3 irradiance = _AmbientLight.rgb + SampleTextureCube(_IrradianceMap, _IrradianceMapSampler, N).rgb * _SkyLightIntensity;
    float NdotV = max(dot(N, V), 0.0);

    #if defined(DIFFUSE_BRDF_LAMBERT)
    gi.diffuseLighting = DiffuseIBL_Lambert(irradiance, surface.color);
    #elif defined(DIFFUSE_BRDF_BURLEY)
    gi.diffuseLighting = DiffuseIBL_Burley(irradiance, surface.color, NdotV, surface.roughness);
    #elif defined(DIFFUSE_BRDF_OREN_NAYAR)
    gi.diffuseLighting = DiffuseIBL_OrenNayar(irradiance, surface.color, NdotV, surface.roughness);
    #endif

    #if defined(SPECULAR_BRDF_GGX)
    float mip = surface.roughness * MAX_REFLECTION_LOD;
    vec3 specular = _AmbientLight.rgb + SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, R, mip).rgb * _SkyLightIntensity;
    gi.specularLighting = SpecularIBL_GGX(specular, F, max(dot(N, V), 0.0), surface.roughness);

    float clearCoatMip = surface.clearCoatRoughness * MAX_REFLECTION_LOD;
    vec3 clearCoatSpecular = _AmbientLight.rgb + SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, R, clearCoatMip).rgb * _SkyLightIntensity;
    clearCoatSpecular *= 2.5;

    vec3 coatPrefilter = clearCoatSpecular;// SpecularIBL_GGX(gi.clearCoatSpecular, F, max(dot(N,V),0.0), surfaceWS.clearCoatRoughness); 
    vec3 coatF = ClearCoatFresnel(max(dot(N, V), 0.0), clearCoatIOR);
    vec3 clearCoatSpec = coatPrefilter * coatF * surface.clearCoat;
    gi.clearCoatSpecularLighting = clearCoatSpec;
    #endif

    #if defined(SPECULAR_BRDF_GGX_CUSTOM)
    float mip = surface.roughness * MAX_REFLECTION_LOD;
    vec3 specular = _AmbientLight.rgb + SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, R, mip).rgb * _SkyLightIntensity;
    gi.specularLighting = SpecularIBL_SkinGGX(specular, F, max(dot(N, V), 0.0), surface.roughness);
    gi.clearCoatSpecularLighting = vec3(0);
    #endif

    return gi;
}

vec3 EvaluateDirectLight(Surface surface, Light light){
    vec3 N = normalize(surface.normal); //surface.normal;
    vec3 V = normalize(surface.viewDirection); //surface.viewDirection;
    vec3 L = normalize(light.direction); //light.direction;

    float metallic = surface.metallic;
    float roughness = surface.roughness;

    vec3 albedo = surface.color;
    vec3 radiance = light.color * light.attenuation;

    // Material F0
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // BRDFs
    vec3 H = normalize(V + L);
    vec3 F = fresnelSchlick( max(dot(H, V), 0.0), F0);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    
    #if defined(DIFFUSE_BRDF_LAMBERT)
    vec3 diffuse = kD * DiffuseBRDF_Lambert(albedo); //DiffuseBRDF_Lambert(albedo, F, metallic);
    #elif defined(DIFFUSE_BRDF_BURLEY)
    vec3 diffuse = kD * DiffuseBRDF_Burley(N, V, L, albedo, roughness);
    #elif defined(DIFFUSE_BRDF_OREN_NAYAR)
    vec3 diffuse = kD * DiffuseBRDF_OrenNayar(N, V, L, albedo, roughness);
    #endif

    #if defined(SPECULAR_BRDF_GGX)
    vec3 specular = SpecularBRDF_GGX(N, V, L, F0, roughness);
    vec3 coat = ClearCoatBRDF_GGX(surface, L);
    #endif

    #if defined(SPECULAR_BRDF_GGX_CUSTOM)
    vec3 specular = SpecularBRDF_SkinGGX(N, V, L, F0, roughness);
    vec3 coat = ClearCoatBRDF_GGX(surface, L);
    #endif

    // Energy loss through coat
    vec3 base = diffuse + specular;
    base *= 1.0 - surface.clearCoat * 0.25;

    float NdotL = max(dot(N, L), 0.0);

    return (base + coat * surface.clearCoat) * radiance * NdotL;
}

vec3 EvaluateIndirectLight(Surface surfaceWS, GI gi){
    vec3 N = surfaceWS.normal;
    vec3 V = surfaceWS.viewDirection;

    //Base Fresnel
    vec3 F0 = mix(vec3(0.04), surfaceWS.color, surfaceWS.metallic);

    vec3 F = fresnelSchlickRoughness(max(dot(N,V),0.0), F0, surfaceWS.roughness);
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - surfaceWS.metallic);

    vec3 diffuse = gi.diffuseLighting;

    //Base specular IBL
    //vec3 R = reflect(-V, N);
    vec3 specular = gi.specularLighting;// SpecularIBL_GGX(gi, F, max(dot(N,V),0.0), surfaceWS.roughness);

    //vec3 coatPrefilter = gi.clearCoatSpecular;// SpecularIBL_GGX(gi.clearCoatSpecular, F, max(dot(N,V),0.0), surfaceWS.clearCoatRoughness); 
    //vec3 coatF = ClearCoatFresnel(max(dot(N,V),0.0), clearCoatIOR);
    vec3 clearCoatSpec = gi.clearCoatSpecularLighting; //coatPrefilter * coatF * surfaceWS.clearCoat;

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
////////////////////////////////////

vec3 GetFinalColor(Surface surfaceWS){
    ShadowData shadowData = GetShadowData(surfaceWS);

    GI gi = GetGI(surfaceWS);
	
	vec3 color = EvaluateIndirectLight(surfaceWS, gi);// * surfaceWS.color * surfaceWS.occlusion;

	for(int i = 0; i < GetDirectionalLightCount(); i++){
		Light light = GetDirectionalLight(i, surfaceWS, shadowData);
		color += EvaluateDirectLight(surfaceWS, light);
	}
	for(int j = 0; j < GetOtherLightCount(); j++){
		Light light = GetOtherLight(j, surfaceWS, shadowData);
		color += EvaluateDirectLight(surfaceWS, light);
	}

	//color = color / (color + vec3(1.0));
	//color = pow(color, vec3(1.0/2.2)); 

	return color;
}

//////////////New///////////

struct LightingResult{
    vec3 diffuse;
    vec3 specular;
};

LightingResult IncomingLightNew(Surface surface, Light light){
    LightingResult r;

    vec3 albedo = surface.color;
    float metallic = surface.metallic;
    float roughness = surface.roughness;

    vec3 N = surface.normal;
    vec3 V = surface.viewDirection;
    vec3 L = light.direction;
    vec3 H = normalize(V + L);

    vec3 radiance = light.color * light.attenuation;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);

    vec3 spec = (NDF * G * F) /
        (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);

    r.diffuse  = kD * albedo / PI * radiance * NdotL;
    r.specular = spec * radiance * NdotL;

    return r;
}

LightingResult AmbientLightNew(Surface s){
    LightingResult r;

    vec3 F0 = mix(vec3(0.04), s.color, s.metallic);
    vec3 F = fresnelSchlickRoughness(
        max(dot(s.normal, s.viewDirection), 0.0),
        F0, s.roughness
    );

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - s.metallic);

    vec3 irradiance = SampleTextureCube(_IrradianceMap, _IrradianceMapSampler, s.normal).rgb;
    vec3 diffuse = irradiance * s.color;

    float mip = s.roughness * MAX_REFLECTION_LOD;
    vec3 R = reflect(-s.viewDirection, s.normal);
    vec3 prefiltered = SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, R, mip).rgb;

    vec2 brdf = SampleTexture2D(_BrdfLUT, _BrdfLUTSampler,
        vec2(max(dot(s.normal, s.viewDirection), 0.0), s.roughness)).rg;

    vec3 specular = prefiltered * (F * brdf.x + brdf.y);

    r.diffuse  = kD * diffuse * s.occlusion;
    r.specular = specular * s.occlusion;

    return r;
}

LightingResult GetFinalLighting(Surface surfaceWS){
    ShadowData shadowData = GetShadowData(surfaceWS);

    LightingResult total;
    total.diffuse = vec3(0.0);
    total.specular = vec3(0.0);

    LightingResult amb = AmbientLightNew(surfaceWS);
    total.diffuse  += amb.diffuse;
    total.specular += amb.specular;

    for(int i = 0; i < GetDirectionalLightCount(); i++){
        Light light = GetDirectionalLight(i, surfaceWS, shadowData);
        LightingResult r = IncomingLightNew(surfaceWS, light);
        total.diffuse  += r.diffuse;
        total.specular += r.specular;
    }

    for(int j = 0; j < GetOtherLightCount(); j++){
        Light light = GetOtherLight(j, surfaceWS, shadowData);
        LightingResult r = IncomingLightNew(surfaceWS, light);
        total.diffuse  += r.diffuse;
        total.specular += r.specular;
    }

    return total;
}

#endif