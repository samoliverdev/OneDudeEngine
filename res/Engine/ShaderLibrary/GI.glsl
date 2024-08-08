#ifndef GI_INCLUDED
#define GI_INCLUDED

uniform vec3 _AmbientLight = vec3(0.1, 0.1, 0.1);
uniform vec3 _IrradianceMapScale = vec3(0, 0, 0);
uniform samplerCube _IrradianceMap;
uniform samplerCube _PrefilterMap; 
uniform float _SkyLightIntensity;

vec3 SampleEnvironmentDiffuse(Surface surfaceWS){
    vec4 environment = texture(_IrradianceMap, surfaceWS.normal);
    return _AmbientLight + (environment.rgb * _IrradianceMapScale);
}

float PerceptualRoughnessToMipmapLevel(float perceptualRoughness){
    const float MAX_REFLECTION_LOD = 4.0;
    perceptualRoughness = perceptualRoughness * (1.7 - 0.7 * perceptualRoughness);
    return perceptualRoughness * MAX_REFLECTION_LOD;
}

vec3 SampleEnvironmentSpecular(Surface surfaceWS, BRDF brdf){
    vec3 uvw = reflect(-surfaceWS.viewDirection, surfaceWS.normal);
    float mip = PerceptualRoughnessToMipmapLevel(brdf.perceptualRoughness);
    vec3 environment = textureLod(_PrefilterMap, uvw, mip).rgb;
	return environment.rgb;
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