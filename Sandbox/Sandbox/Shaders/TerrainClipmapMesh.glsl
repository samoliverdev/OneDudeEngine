#version 330 core

#pragma BeginProperties
    Color4 color
    Texture2D mainTex White
    Texture2D heightMap Back
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
    mat3 normalMatrix;
    flat vec2 uvOffset_;
} vsOut;

uniform sampler2D heightMap;
uniform float heightScale;
uniform vec3 viewPos;
uniform float metersPerHeightfieldTexel = 1;
uniform vec2 uvOffset = vec2(0);

vec3 roundToIncrement(vec3 value, float increment) {
    return round(value * (1.0 / increment)) * increment;
}

void main(){
    mat4 targetModelMatrix = GetModelMatrix();
    vec3 localPos = GetLocalPos().xyz;

    /*float snapStep = 256*2;
    int div = 256*100;
    vec3 camPos = viewPos.xyz;
    vec3 snapCamera = roundToIncrement(camPos, snapStep);
    snapCamera.y = 0;
    localPos += snapCamera;
    vsOut.uvOffset_ = vec2((snapCamera.x/div), -(snapCamera.z/div));*/ 

    /*vec3 wsCamera = viewPos.xyz;
    float gridLevel = localPos.y;
    float mipMetersPerHeightfieldTexel = metersPerHeightfieldTexel * exp2(gridLevel);
    vec2 objectToWorld = roundToIncrement(wsCamera.xz, mipMetersPerHeightfieldTexel);
    vec3 wsPosition = vec3(localPos.x * metersPerHeightfieldTexel + objectToWorld.x, 0, -(localPos.z * metersPerHeightfieldTexel + objectToWorld.y));

    localPos = wsPosition;*/

    float height = texture(heightMap, texCoord + uvOffset).r;
    localPos.y = height * heightScale;

    vsOut.pos = localPos;
    vsOut.normal = normal;
    vsOut.texCoord = texCoord;
    vsOut.worldPos = vec3(targetModelMatrix * vec4(pos, 1.0));
    //worldNormal = vec3(model * vec4(normal, 1.01));
    vsOut.worldNormal = mat3(transpose(inverse(targetModelMatrix))) * normal; // for non-uniform scale objects
    vsOut.normalMatrix = mat3(transpose(inverse(targetModelMatrix)));

    gl_Position = projection * view * targetModelMatrix * vec4(localPos, 1.0);
}
#endif

#ifdef FRAGMENT

uniform mat4 view;

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
    mat3 normalMatrix;
    flat vec2 uvOffset_;
} fsIn;

uniform vec3 viewPos;

uniform sampler2D heightMap;
uniform float heightScale;
uniform vec2 uvOffset = vec2(0);

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

float ddx(float v){
    return dFdx(v);
}

float ddy(float v){
    return dFdy(v);
}

vec3 ddx(vec3 v){
    return vec3(dFdx(v.x), dFdx(v.y), dFdx(v.z));
}

vec3 ddy(vec3 v){
    return vec3(dFdy(v.x), dFdy(v.y), dFdy(v.z));
}

vec3 HeightToNormal(float height, vec3 normal, vec3 pos){
    vec3 worldDirivativeX = ddx(pos);
    vec3 worldDirivativeY = ddy(pos);
    vec3 crossX = cross(normal, worldDirivativeX);
    vec3 crossY = cross(normal, worldDirivativeY);
    float d = abs(dot(crossY, worldDirivativeX));
    vec3 inToNormal = ((((height + ddx(height)) - height) * crossY) + (((height + ddy(height)) - height) * crossX)) * sign(d);
    inToNormal.y *= -1.0;
    return normalize((d * normal) - inToNormal);
}

vec3 NormalStrength(vec3 In, float Strength){
    return vec3(In.rg * Strength, mix(1, In.b, clamp(Strength, 0, 1)));
}

//Source: https://forum.unity.com/threads/calculate-vertex-normals-in-shader-from-heightmap.169871/
vec3 filterNormalLod(vec2 uv){
    vec2 texSize = textureSize(heightMap, 0);
    vec2 texelSize = vec2(1.0 / texSize.x, 1.0 / texSize.y);
    
    float h0 = texture(heightMap, uv + ( vec2( 0,-1) * texelSize) ).r * (heightScale/1);
    float h1 = texture(heightMap, uv + ( vec2(-1, 0) * texelSize) ).r * (heightScale/1);
    float h2 = texture(heightMap, uv + ( vec2( 1, 0) * texelSize) ).r * (heightScale/1);
    float h3 = texture(heightMap, uv + ( vec2( 0, 1) * texelSize) ).r * (heightScale/1);

    return normalize(vec3(h1 - h2, 2, h0 - h3));
}

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
	return 1.0;
    //return GetMask(baseUV).g;

    /*float strength = occlusion;
	float _occlusion = GetMask(baseUV).g;
	_occlusion = mix(_occlusion, 1.0, strength);
	return _occlusion;*/
}

void main(){
    vec4 base = texture(mainTex, fsIn.texCoord + uvOffset);
    base = base * color;

    //float height = texture(heightMap, fsIn.texCoord + fsIn.uvOffset_).r * heightScale;

    Surface surface;
    surface.position = fsIn.worldPos;
    //surface.normal = normalize(fsIn.worldNormal);
    //surface.normal = NormalStrength(HeightToNormal(height, fsIn.worldNormal, fsIn.worldPos), 0.5f);
    surface.normal = NormalStrength(filterNormalLod(fsIn.texCoord + uvOffset), 1);
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