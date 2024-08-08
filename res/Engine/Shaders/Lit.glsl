#version 330 core

#pragma BeginProperties
    Color4 color
    Texture2D mainTex White
    Texture2D normal Normal
    Texture2D emissionMap Black
    Color4 emissionColor
    Texture2D maskMap White
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

#include res/Engine/ShaderLibrary/Vertex.glsl

out VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
    mat3 TBN;
} vsOut;

void main(){
    mat4 targetModelMatrix = GetModelMatrix();

    vec3 T = normalize(vec3(targetModelMatrix * vec4(tangents, 0.0)));
    vec3 B = normalize(vec3(targetModelMatrix * vec4(cross(tangents, normal), 0.0)));
    vec3 N = normalize(vec3(targetModelMatrix * vec4(normal, 0.0)));
    
    vsOut.pos = pos;
    vsOut.normal = normal;
    vsOut.texCoord = texCoord;
    vsOut.TBN = mat3(T, B, N);
    vsOut.worldPos = vec3(targetModelMatrix * vec4(pos, 1.0));
    //vsOut.worldNormal = vec3(targetModelMatrix * vec4(normal, 0));
    vsOut.worldNormal = mat3(transpose(inverse(targetModelMatrix))) * normal; // for non-uniform scale objects

    gl_Position = projection * view * targetModelMatrix * GetLocalPos();
}
#endif

#ifdef FRAGMENT

uniform mat4 view;

#include res/Engine/ShaderLibrary/Core.glsl
#include res/Engine/ShaderLibrary/Common.glsl
#include res/Engine/ShaderLibrary/Surface.glsl
#include res/Engine/ShaderLibrary/Shadows.glsl
#include res/Engine/ShaderLibrary/Light.glsl
#include res/Engine/ShaderLibrary/BRDF.glsl
#include res/Engine/ShaderLibrary/GI.glsl
#include res/Engine/ShaderLibrary/Lighting.glsl

in VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
    mat3 TBN;
} fsIn;

uniform vec3 viewPos;

uniform vec4 color = vec4(1,1,1,1);
uniform sampler2D mainTex;
uniform sampler2D normal;
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
	//return 1.0;

    float strength = occlusion;
	float _occlusion = GetMask(baseUV).g;
	_occlusion = mix(_occlusion, 1.0, strength);
	return _occlusion;
}

void main(){
    vec4 base = textureSRGB(mainTex, fsIn.texCoord);
    base = base * color;

    vec4 normalMap = texture(normal, fsIn.texCoord);
    vec3 _normal = normalize(normalMap.rgb * 2.0 - 1.0); // transforms from [-1,1] to [0,1] 
    _normal = normalize(fsIn.TBN * _normal); 

    Surface surface;
    surface.position = fsIn.worldPos;
    surface.normal = _normal;
    surface.viewDirection = normalize(viewPos - fsIn.worldPos);
    surface.depth = -(view * vec4(fsIn.worldPos, 1)).z;
    surface.color = base.rgb;
    surface.alpha = base.a;
    surface.occlusion = GetOcclusion(fsIn.texCoord);
    surface.metallic = GetMetallic(fsIn.texCoord);
    surface.smoothness = GetSmoothness(fsIn.texCoord);
    
    BRDF brdf = GetBRDF(surface);
    GI gi = GetGI(surface, brdf);
    vec3 color = GetLighting(surface, brdf, gi);
    color += GetEmission(fsIn.texCoord);
    fragColor = vec4(color, surface.alpha);

    if(base.a < cutoff) discard;
}
#endif

#pragma EndPass 