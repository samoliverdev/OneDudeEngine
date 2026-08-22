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

vec3 TangentToWorld(vec3 H, vec3 N){
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

vec3 ImportanceSampleCosine(vec2 Xi, vec3 N){
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt(1.0 - Xi.y);
    float sinTheta = sqrt(Xi.y);

    vec3 H;

    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    return TangentToWorld(H, N);
}

float RadicalInverse_VdC(uint bits){
    bits =
        (bits << 16u) |
        (bits >> 16u);

    bits =
        ((bits & 0x55555555u) << 1u) |
        ((bits & 0xAAAAAAAAu) >> 1u);

    bits =
        ((bits & 0x33333333u) << 2u) |
        ((bits & 0xCCCCCCCCu) >> 2u);

    bits =
        ((bits & 0x0F0F0F0Fu) << 4u) |
        ((bits & 0xF0F0F0F0u) >> 4u);

    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N){
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 SampleEnvironment(vec3 L){
    //return vec3(0);
    return _AmbientLight.rgb + SampleTextureCube(_IrradianceMap, _IrradianceMapSampler, L).rgb * _SkyLightIntensity;
    //return _AmbientLight.rgb + SampleTextureCube(_EnvironmentMap, _EnvironmentMapSampler, L).rgb * _SkyLightIntensity;
}

/*vec3 DiffuseBRDF_Burley(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness){
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    vec3 H = normalize(V + L);

    float LdotH = max(dot(L, H), 0.0);

    float FD90 = 0.5 + 2.0 * LdotH * LdotH * roughness;

    float FL = pow(1.0 - NdotL, 5.0);

    float FV = pow(1.0 - NdotV, 5.0);

    float diffuse = (1.0 + (FD90 - 1.0) * FL) * (1.0 + (FD90 - 1.0) * FV);

    return albedo * diffuse / PI;
}*/

vec3 DiffuseBRDF_Burley(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness){
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    if(NdotL <= 0.0 || NdotV <= 0.0) return vec3(0.0);

    vec3 VplusL = V + L;
    float len2 = dot(VplusL, VplusL);

    if(len2 < 1e-6) return vec3(0.0);

    vec3 H = VplusL * inversesqrt(len2);

    float LdotH = max(dot(L, H), 0.0);

    float FD90 = 0.5 + 2.0 * LdotH * LdotH * roughness;

    float FL = pow(1.0 - NdotL, 5.0);
    float FV = pow(1.0 - NdotV, 5.0);

    float FD = (1.0 + (FD90 - 1.0) * FL) * (1.0 + (FD90 - 1.0) * FV);

    return albedo * FD / PI;
}

vec3 DiffuseIBL_Burley(vec3 N, vec3 V, vec3 albedo, float roughness){
    vec3 result = vec3(0.0);
    const int SAMPLE_COUNT = 32;

    for(int i = 0; i < SAMPLE_COUNT; i++){
        vec2 Xi = Hammersley(uint(i), uint(SAMPLE_COUNT));

        vec3 L = ImportanceSampleCosine(Xi, N);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL <= 0.0) continue;

        vec3 environment = SampleEnvironment(L);
        vec3 brdf = DiffuseBRDF_Burley(N, V, L, albedo, roughness);
        result += brdf * environment * PI;
    }

    return result / float(SAMPLE_COUNT);

    /*vec3 result = vec3(0.0);
    const int SAMPLE_COUNT = 32;

    for(int i = 0; i < SAMPLE_COUNT; i++){
        vec2 Xi = Hammersley(uint(i), uint(SAMPLE_COUNT));
        vec3 L = ImportanceSampleCosine(Xi, N);

        float NdotL = max(dot(N, L), 0.0);

        if(NdotL <= 0.0) continue;

        vec3 H = normalize(V + L);

        float NdotV = max(dot(N, V), 0.0);
        float LdotH = max(dot(L, H), 0.0);

        float FD90 = 0.5 + 2.0 * LdotH * LdotH * roughness;

        float FL = pow(1.0 - NdotL, 5.0);
        float FV = pow(1.0 - NdotV, 5.0);
        float FD = (1.0 + (FD90 - 1.0) * FL) * (1.0 + (FD90 - 1.0) * FV);

        vec3 brdf = albedo * FD / PI;

        vec3 environment = SampleEnvironment(L);

        // Because cosine-weighted sampling has:
        //
        // PDF = NdotL / PI
        //
        // BRDF * NdotL / PDF
        // = BRDF * PI

        result += brdf * environment * PI;
    }

    return result / float(SAMPLE_COUNT);*/
}

///////////////////// OrenNayar //////////////////////
/*vec3 DiffuseBRDF_OrenNayar(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness){
    float sigma = roughness * (PI * 0.5);

    float sigma2 = sigma * sigma;

    float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));

    float B = 0.45 * (sigma2 / (sigma2 + 0.09));

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float thetaI = acos(NdotL);
    float thetaR = acos(NdotV);

    float alpha = max(thetaI, thetaR);
    float beta  = min(thetaI, thetaR);

    vec3 Lp = normalize(L - N * NdotL);
    vec3 Vp = normalize(V - N * NdotV);

    float gamma = max(dot(Lp, Vp), 0.0);

    return albedo / PI * (A + B * gamma * sin(alpha) * tan(beta));
}*/

vec3 DiffuseBRDF_OrenNayar(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness){
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    if(NdotL <= 0.0 || NdotV <= 0.0) return vec3(0.0);

    float sigma = roughness * (PI * 0.5);
    float sigma2 = sigma * sigma;

    float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
    float B = 0.45 * (sigma2 / (sigma2 + 0.09));

    float alpha = max(acos(clamp(NdotL, 0.0, 1.0)), acos(clamp(NdotV, 0.0, 1.0)));
    float beta = min(acos(clamp(NdotL, 0.0, 1.0)), acos(clamp(NdotV, 0.0, 1.0)));

    vec3 Lp = L - N * NdotL;
    vec3 Vp = V - N * NdotV;

    float gamma = 0.0;

    float Lp2 = dot(Lp, Lp);
    float Vp2 = dot(Vp, Vp);

    if(Lp2 > 1e-6 && Vp2 > 1e-6){
        Lp *= inversesqrt(Lp2);
        Vp *= inversesqrt(Vp2);

        gamma = max(dot(Lp, Vp), 0.0);
    }

    // Prevent grazing-angle numerical explosion.
    float tanBeta = sin(beta) / max(cos(beta), 1e-4);
    float diffuse = A + B * gamma * sin(alpha) * tanBeta;
    return albedo * diffuse / PI;
}

vec3 DiffuseIBL_OrenNayar(vec3 N, vec3 V, vec3 albedo, float roughness){
    vec3 result = vec3(0.0);

    const int SAMPLE_COUNT = 32;

    for(int i = 0; i < SAMPLE_COUNT; i++){
        vec2 Xi = Hammersley(uint(i), uint(SAMPLE_COUNT));

        vec3 L = ImportanceSampleCosine(Xi, N);

        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);

        if(NdotL <= 0.0) continue;

        // Roughness -> Oren-Nayar sigma
        float sigma = roughness * (PI * 0.5);

        float sigma2 = sigma * sigma;

        float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
        float B = 0.45 * (sigma2 / (sigma2 + 0.09));

        float thetaI = acos(clamp(NdotL, 0.0, 1.0));
        float thetaR = acos(clamp(NdotV, 0.0, 1.0));

        float alpha = max(thetaI, thetaR);
        float beta = min(thetaI, thetaR);

        vec3 Lp = L - N * NdotL;
        vec3 Vp = V - N * NdotV;

        float gamma = 0.0;

        float LpLength = length(Lp);
        float VpLength = length(Vp);

        if(LpLength > 0.0001 && VpLength > 0.0001){
            Lp /= LpLength;
            Vp /= VpLength;

            gamma = max(dot(Lp, Vp), 0.0);
        }

        float oren = A + B * gamma * sin(alpha) * tan(beta);
        vec3 brdf = albedo * oren / PI;

        vec3 environment = SampleEnvironment(L);

        result += brdf * environment * PI;
    }

    return result / float(SAMPLE_COUNT);
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

    #if defined(DIFFUSE_BRDF_LAMBERT)
    vec3 diffuse = _AmbientLight.rgb + SampleTextureCube(_IrradianceMap, _IrradianceMapSampler, N).rgb * _SkyLightIntensity;
    gi.diffuseLighting = DiffuseIBL_Lambert(diffuse, surface.color);
    #endif

    #if defined(DIFFUSE_BRDF_BURLEY)
    gi.diffuseLighting = DiffuseIBL_Burley(N, V, surface.color, surface.roughness);
    #endif

    #if defined(DIFFUSE_BRDF_OREN_NAYAR)
    gi.diffuseLighting = DiffuseIBL_OrenNayar(N, V, surface.color, surface.roughness);
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