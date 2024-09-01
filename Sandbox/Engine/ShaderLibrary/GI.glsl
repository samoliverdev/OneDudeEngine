#ifndef GI_INCLUDED
#define GI_INCLUDED

uniform vec3 _AmbientLight = vec3(0.1, 0.1, 0.1);
uniform vec3 _IrradianceMapScale = vec3(0, 0, 0);
uniform samplerCube _IrradianceMap;
uniform samplerCube _PrefilterMap; 
uniform float _SkyLightIntensity;
uniform sampler2D _BrdfLUT;

const float MAX_REFLECTION_LOD = 4.0;

vec3 SampleEnvironmentDiffuse(Surface surfaceWS){
    vec4 environment = texture(_IrradianceMap, surfaceWS.normal);
    return _AmbientLight + (environment.rgb * _SkyLightIntensity);
}

float PerceptualRoughnessToMipmapLevel(float perceptualRoughness){
    perceptualRoughness = perceptualRoughness * (1.7 - 0.7 * perceptualRoughness);
    return perceptualRoughness * MAX_REFLECTION_LOD;
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness){
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 SampleEnvironmentSpecular(Surface surfaceWS, BRDF brdf){
    vec3 uvw = reflect(-surfaceWS.viewDirection, surfaceWS.normal);
    float mip = PerceptualRoughnessToMipmapLevel(brdf.perceptualRoughness);
    vec3 environment = textureLod(_PrefilterMap, uvw, mip).rgb * _SkyLightIntensity;
    return environment;
    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, surfaceWS.color, surfaceWS.metallic);
    vec3 F = FresnelSchlickRoughness(max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0), F0, brdf.roughness);
    vec2 envBRDF = texture(_BrdfLUT, vec2(max(dot(surfaceWS.normal, surfaceWS.viewDirection), 0.0), brdf.roughness)).rg;
    
    return environment * (F * envBRDF.x + envBRDF.y);
}

struct GI{
    vec3 diffuse;
    vec3 specular;
};

GI GetGI(Surface surfaceWS, BRDF brdf){
    GI gi;
    gi.diffuse = SampleEnvironmentDiffuse(surfaceWS); //_AmbientLight; //SampleLightMap(lightMapUV) + SampleLightProbe(surfaceWS);
    gi.specular = SampleEnvironmentSpecular(surfaceWS, brdf);
    return gi;
}

#endif