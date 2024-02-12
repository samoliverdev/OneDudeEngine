#ifndef GI_INCLUDED
#define GI_INCLUDED

uniform vec3 _AmbientLight = vec3(0.1, 0.1, 0.1);
uniform samplerCube _SpecCube0;

vec3 SampleEnvironment(Surface surfaceWS){
	vec3 uvw = reflect(-surfaceWS.viewDirection, surfaceWS.normal);
	vec4 environment = texture(_SpecCube0, uvw);
	return environment.rgb;
}

struct GI{
    vec3 diffuse;
    vec3 specular;
};

GI GetGI(Surface surfaceWS){
    GI gi;
    gi.diffuse = _AmbientLight; //SampleLightMap(lightMapUV) + SampleLightProbe(surfaceWS);
    gi.specular = SampleEnvironment(surfaceWS);
    return gi;
}

#endif