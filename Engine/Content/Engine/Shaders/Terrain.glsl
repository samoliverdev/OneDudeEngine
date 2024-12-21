#version 330 core

#pragma BeginProperties
    Color4 color
    Texture2D splatmap Black
    Texture2D tex0 White
    Texture2D tex1 White
    Texture2D tex2 White
    Texture2D tex3 White
    Texture2D tex4 White
    Texture2D mainTex White
    Texture2D heightMap Back
    Texture2D heightMapNormal Back
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

#pragma CullFace NONE
#pragma DepthTest LESS
#pragma Blend Off

#ifdef VERTEX
#include Engine/ShaderLibrary/Vertex.glsl

out VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
    mat3 normalMatrix;
    flat vec2 uvOffset_;
    mat3 TBN;
} vsOut;

uniform sampler2D heightMap;

uniform float heightScale;
uniform vec3 viewPos;
uniform float metersPerHeightfieldTexel = 1;
uniform vec2 uvOffset = vec2(0);

uniform vec2 heightmapTilling = vec2(1, 1);
uniform vec2 heightmapOffset = vec2(0, 0);

void main(){
    mat4 targetModelMatrix = GetModelMatrix();
    vec3 localPos = GetLocalPos().xyz;

    vec3 T = normalize(vec3(targetModelMatrix * vec4(tangents, 0.0)));
    vec3 B = normalize(vec3(targetModelMatrix * vec4(cross(tangents, normal), 0.0)));
    vec3 N = normalize(vec3(targetModelMatrix * vec4(normal, 0.0)));
    vsOut.TBN = mat3(T, B, N);

    vec2 uv = texCoord;
    //uv.y = 1 - uv.y;

    float height = texture(heightMap, uv * heightmapTilling + heightmapOffset).r; // uv + uvOffset
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

#include Engine/ShaderLibrary/Common.glsl
#include Engine/ShaderLibrary/Surface.glsl
#include Engine/ShaderLibrary/Shadows.glsl
#include Engine/ShaderLibrary/Light.glsl
#include Engine/ShaderLibrary/BRDF.glsl
#include Engine/ShaderLibrary/GI.glsl
#include Engine/ShaderLibrary/Lighting.glsl

in VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
    mat3 normalMatrix;
    flat vec2 uvOffset_;
    mat3 TBN;
} fsIn;

uniform vec3 viewPos;

uniform float heightScale;
uniform sampler2D heightMap;
uniform sampler2D heightMapNormal;
uniform sampler2D heightMapNorth;
uniform sampler2D heightMapWest;

uniform vec2 uvOffset = vec2(0);

uniform sampler2D mainTex;

uniform sampler2D splatmap;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;


uniform vec4 color = vec4(1,1,1,1);
uniform sampler2D emissionMap;
uniform vec4 emissionColor = vec4(0,0,0,0);
uniform sampler2D maskMap;
uniform float occlusion = 1;
uniform float metallic = 0;
uniform float smoothness = 0.5;
uniform float cutoff  = 0.5;

out vec4 fragColor;

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

vec3 GetNormal(mat3 TBN, vec2 uv){
    vec3 n = texture(heightMapNormal, uv).xyz;
    //return fsIn.normalMatrix * n;

    n = n * 2.0 - 1.0;
    //n.xy *= normalStrength;
    n = normalize(n);
    return normalize(TBN * n);
}

uniform vec2 heightmapTilling = vec2(1, 1);
uniform vec2 heightmapOffset = vec2(0, 0);

void main(){
    //vec4 base = texture(mainTex, fsIn.texCoord + uvOffset);
    vec4 base = texture(mainTex, fsIn.texCoord * heightmapTilling + heightmapOffset);
    base = base * color;
    //base = color;

    vec4 splatmap = texture(splatmap, fsIn.texCoord * heightmapTilling + heightmapOffset);
    base = mix(
        texture(tex0, fsIn.texCoord * heightmapTilling + heightmapOffset),
        texture(tex1, fsIn.texCoord * heightmapTilling + heightmapOffset),
        splatmap.r
    );
    base = mix(
        base,
        texture(tex2, fsIn.texCoord * heightmapTilling + heightmapOffset),
        splatmap.g
    );
    base = mix(
        base,
        texture(tex3, fsIn.texCoord * heightmapTilling + heightmapOffset),
        splatmap.b
    );
    /*base = mix(
        base,
        texture(tex4, fsIn.texCoord * heightmapTilling + heightmapOffset),
        splatmap.a
    );*/

    //float height = texture(heightMap, fsIn.texCoord + fsIn.uvOffset_).r * 1;
    //base = vec4(height, height, height, 1);
    //base = vec4((fsIn.texCoord + uvOffset), 0, 1);

    Surface surface;
    surface.position = fsIn.worldPos;
    //surface.normal = normalize(fsIn.worldNormal);
    surface.normal = NormalStrength(filterNormalLod(fsIn.texCoord * heightmapTilling + heightmapOffset), 1);
    //surface.normal = NormalStrength(GetNormal(fsIn.TBN, fsIn.texCoord * heightmapTilling + heightmapOffset), 1);
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