#ifndef GI_INCLUDED
#define GI_INCLUDED

/*uniform vec3 _AmbientLight;
uniform vec3 _IrradianceMapScale;
uniform samplerCube _IrradianceMap;
uniform samplerCube _PrefilterMap; 
uniform float _SkyLightIntensity;
uniform sampler2D _BrdfLUT;*/

const float MAX_REFLECTION_LOD = 4.0;

vec3 SampleEnvironmentDiffuse(Surface surfaceWS){
    return _AmbientLight + SampleTextureCube(_IrradianceMap, _IrradianceMapSampler, surfaceWS.normal).rgb * _SkyLightIntensity;

    vec4 environment = SampleTextureCube(_IrradianceMap, _IrradianceMapSampler, surfaceWS.normal);
    return _AmbientLight + (environment.rgb * _SkyLightIntensity);
}

float PerceptualRoughnessToMipmapLevel(float perceptualRoughness){
    return perceptualRoughness * MAX_REFLECTION_LOD;

    perceptualRoughness = perceptualRoughness * (1.7 - 0.7 * perceptualRoughness);
    return perceptualRoughness * MAX_REFLECTION_LOD;
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness){
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 SampleEnvironmentSpecular(Surface surfaceWS, BRDF brdf){
    /*
    vec3 F0 = vec3(0.04); 
	F0 = mix(F0, surfaceWS.color, surfaceWS.metallic);
    vec3 R = reflect(-surfaceWS.viewDirection, surfaceWS.normal); 
    vec3 F = FresnelSchlickRoughness(max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0), F0, brdf.roughness);
    float mip = brdf.perceptualRoughness * MAX_REFLECTION_LOD;   
    vec3 prefilteredColor = _AmbientLight + SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, R, mip).rgb * _SkyLightIntensity;   
    vec2 _brdf = SampleTexture2D(_BrdfLUT, _BrdfLUTSampler, vec2(max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0), brdf.roughness)).rg;
    return (prefilteredColor * (F * _brdf.x + _brdf.y));
    //return _AmbientLight + (prefilteredColor * (F * _brdf.x + _brdf.y));
    */

    ///*
    vec3 uvw = reflect(-surfaceWS.viewDirection, surfaceWS.normal);
    float mip = PerceptualRoughnessToMipmapLevel(brdf.perceptualRoughness);
    vec3 environment = SampleTextureCubeLod(_PrefilterMap, _PrefilterMapSampler, uvw, mip).rgb * _SkyLightIntensity; //textureLod(_PrefilterMap, uvw, mip).rgb * _SkyLightIntensity;
    //return environment;
    //return _AmbientLight + environment;
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, surfaceWS.color, surfaceWS.metallic);
    vec3 F = FresnelSchlickRoughness(max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0), F0, brdf.roughness);
    vec2 envBRDF = SampleTexture2D(_BrdfLUT, _BrdfLUTSampler, vec2(max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0), brdf.roughness)).rg;
    return _AmbientLight + (environment * (F * envBRDF.x + envBRDF.y));
    //*/
}

struct GI{
    vec3 diffuse;
    vec3 specular;
};

GI GetGI(Surface surfaceWS, BRDF brdf){
    GI gi;
    gi.diffuse = SampleEnvironmentDiffuse(surfaceWS); //_AmbientLight; //SampleLightMap(lightMapUV) + SampleLightProbe(surfaceWS);
    gi.specular = SampleEnvironmentSpecular(surfaceWS, brdf);
    //gi.diffuse = _AmbientLight;
    //gi.specular = _AmbientLight;
    return gi;
}

#endif