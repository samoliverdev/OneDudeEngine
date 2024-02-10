#version 330 core

#pragma BeginProperties
    Texture2D mainTex White
    Texture2D normal Normal
    Texture2D emissionMap Black
    Color4 emissionColor
    Color4 color
    texture2D maskMap White
    Float occlusion 1 0 1
    Float metallic 0 0 1
    Float smoothness 0.5 0.0 1.0
    Float cutoff 0.5 0 1
#pragma EndProperties

#pragma BeginPass 

#pragma SupportInstancing true
#pragma DrawType Stand
#pragma MultiCompile _ SKINNED INSTANCING
#pragma MultiCompile Opaque Blend

#pragma CullFace BACK
#pragma DepthTest LESS
#pragma Blend Off

#ifdef VERTEX
/*
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
layout (location = 10) in mat4 modelInstancing;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float useInstancing = 0;
*/

#include ../ShaderLibrary/Vertex.glsl

out VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
} vsOut;

void main(){
    //mat4 targetModelMatrix = (useInstancing >= 1.0 ? modelInstancing : model);
    mat4 targetModelMatrix = GetModelMatrix();

    vsOut.pos = pos;
    vsOut.normal = normal;
    vsOut.texCoord = texCoord;
    vsOut.worldPos = vec3(targetModelMatrix * vec4(pos, 1.0));
    //worldNormal = vec3(model * vec4(normal, 1.01));
    vsOut.worldNormal = mat3(transpose(inverse(targetModelMatrix))) * normal; // for non-uniform scale objects

    //gl_Position = projection * view * targetModelMatrix * vec4(pos, 1.0);
    gl_Position = projection * view * targetModelMatrix * GetLocalPos();
}
#endif

#ifdef FRAGMENT

uniform mat4 view;

#include ../ShaderLibrary/Common.glsl
#include ../ShaderLibrary/Surface.glsl
#include ../ShaderLibrary/Shadows.glsl
#include ../ShaderLibrary/Light.glsl
#include ../ShaderLibrary/BRDF.glsl
#include ../ShaderLibrary/GI.glsl
#include ../ShaderLibrary/Lighting.glsl

in VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
} fsIn;

uniform vec3 viewPos;

uniform sampler2D mainTex;
uniform vec4 color = vec4(1,1,1,1);
uniform sampler2D emissionMap;
uniform vec4 emissionColor = vec4(0,0,0,0);
uniform sampler2D maskMap;
uniform float occlusion = 1;
uniform float metallic = 0;
uniform float smoothness = 0.5;
uniform float cutoff  = 0.5;

out vec4 fragColor;

vec3 GetEmission(vec2 baseUV){
	vec4 map = texture(emissionMap, baseUV);
	return map.rgb * emissionColor.rgb;
}

vec4 GetMask(vec2 baseUV){
	return texture(maskMap, baseUV);
}

float GetMetallic(vec2 baseUV){
	float _metallic = metallic;
	_metallic *= GetMask(baseUV).r;
	return _metallic;
}

float GetSmoothness(vec2 baseUV){
	float _smoothness = smoothness;
	_smoothness *= GetMask(baseUV).a;
	return _smoothness;
}

float GetOcclusion(vec2 baseUV){
	return 1;
    //return GetMask(baseUV).g;

    /*float strength = occlusion;
	float _occlusion = GetMask(baseUV).g;
	_occlusion = mix(_occlusion, 1.0, strength);
	return _occlusion;*/
}

void main(){
    vec4 base = texture(mainTex, fsIn.texCoord);
    base = base * color;

    Surface surface;
    surface.position = fsIn.worldPos;
    surface.normal = normalize(fsIn.worldNormal);
    surface.viewDirection = normalize(viewPos - fsIn.worldPos);
    surface.depth = -(view * vec4(fsIn.worldPos, 1)).z;
    surface.color = base.rgb;
    surface.alpha = base.a;
    surface.occlusion = GetOcclusion(fsIn.texCoord);
    surface.metallic = GetMetallic(fsIn.texCoord);
    surface.smoothness = GetSmoothness(fsIn.texCoord);
    
    BRDF brdf = GetBRDF(surface);
    GI gi = GetGI(surface);
    vec3 color = GetLightingFinal2(surface, brdf, gi);
    color += GetEmission(fsIn.texCoord);
    fragColor = vec4(color, surface.alpha);

    if(base.a < cutoff) discard;
}
#endif

#pragma EndPass 