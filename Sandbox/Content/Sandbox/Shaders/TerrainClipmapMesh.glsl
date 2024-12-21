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

vec3 roundToIncrement(vec3 value, float increment) {
    return round(value * (1.0 / increment)) * increment;
}

uniform vec2 heightmapTilling = vec2(1, 1);
uniform vec2 heightmapOffset = vec2(0, 0);

void main(){
    mat4 targetModelMatrix = GetModelMatrix();
    vec3 localPos = GetLocalPos().xyz;

    vec3 T = normalize(vec3(targetModelMatrix * vec4(tangents, 0.0)));
    vec3 B = normalize(vec3(targetModelMatrix * vec4(cross(tangents, normal), 0.0)));
    vec3 N = normalize(vec3(targetModelMatrix * vec4(normal, 0.0)));
    vsOut.TBN = mat3(T, B, N);

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
uniform sampler2D heightMapNorth;
uniform sampler2D heightMapWest;
uniform sampler2D heightMapLeft;

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
    vec3 worldDirivativeX = ddx(pos * 10);
    vec3 worldDirivativeY = ddy(pos * 10);
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

vec3 GetPoint(vec2 uv){
    vec2 texSize = textureSize(heightMap, 0);

    if(uv.x > 1.0){
        return vec3(
            uv.x * texSize.x, 
            uv.y * texSize.y,
            texture(heightMapWest, vec2(uv.x-1.0, uv.y)).r * heightScale
        );
    }

    return vec3(
        uv.x * texSize.x, 
        uv.y * texSize.y,
        texture(heightMap, uv).r * heightScale
    );
}

vec3 GetNormal(vec2 uv){
    vec2 texSize = textureSize(heightMap, 0);
    vec2 texelSize = vec2(1.0 / texSize.x, 1.0 / texSize.y) * 1;

    vec3 o = GetPoint(uv),
         a = GetPoint(uv + (vec2( 1, 0) * texelSize)),
         b = GetPoint(uv + (vec2( 0, 1) * texelSize)),
         c = GetPoint(uv + (vec2( -1, 0) * texelSize)),
         d = GetPoint(uv + (vec2( 0, -1) * texelSize));
    vec3 n1 = normalize(cross(a-o, b-o));
    vec3 n2 = normalize(cross(b-o, c-o));
    vec3 n3 = normalize(cross(c-o, d-o));
    vec3 n4 = normalize(cross(d-o, a-o));

    //return normalize((n1+n2+n3+n4));

    float W = texSize.x;
    float H = texSize.y;
    vec3 sum = {0,0,0};
    bool b1 = (o.x+1 >= 0 && o.y >= 0 && o.x+1 < W && o.y < H);
    bool b2 = (o.x >= 0 && o.y+1 >= 0 && o.x < W && o.y+1 < H);
    bool b3 = (o.x-1 >= 0 && o.y >= 0 && o.x-1 < W && o.y < H);
    bool b4 = (o.x >= 0 && o.y-1 >= 0 && o.x < W && o.y-1 < H);
    if(b1 && b2) sum += n1;
    if(b2 && b3) sum += n2;
    if(b3 && b4) sum += n3;
    if(b4 && b1) sum += n4;

    //if(length(sum) <= 0) return vec3(0, 1, 0);
    return normalize(sum);
}

vec3 GetNormal2(vec2 uv, float adjacentDistance){
    vec2 texSize = textureSize(heightMap, 0);
    vec2 texelSize = vec2(1.0 / texSize.x, 1.0 / texSize.y) * 1;

    vec3 vertex = GetPoint(uv);
    vec3 westVert = GetPoint(uv + (vec2(adjacentDistance, 0) * texelSize));
    vec3 northVert = GetPoint(uv + (vec2(0, adjacentDistance) * texelSize));
    return -normalize(
        cross(
            northVert - vertex,
            westVert - vertex
        )
    );
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

float getHeight(vec2 uv){
    if(uv.x > 1.0) return texture(heightMapWest, vec2(uv.x-1.0, uv.y)).r;
    if(uv.x < 0) return texture(heightMapLeft, vec2(uv.x+1.0, uv.y)).r;
    return texture(heightMap, uv).r;
}

vec3 filterNormalLod2(vec2 uv){
    vec2 texSize = textureSize(heightMap, 0);
    vec2 texelSize = vec2(1.0 / texSize.x, 1.0 / texSize.y);
    
    float h0 = getHeight(uv + ( vec2( 0,-1) * texelSize) ) * (heightScale/1);
    float h1 = getHeight(uv + ( vec2(-1, 0) * texelSize) ) * (heightScale/1);
    float h2 = getHeight(uv + ( vec2( 1, 0) * texelSize) ) * (heightScale/1);
    float h3 = getHeight(uv + ( vec2( 0, 1) * texelSize) ) * (heightScale/1);

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

vec3 toNormalmap(vec3 n){
    n *= vec3(1.0, 1.0, -1.0);
    n = n / 2.0 + 0.5;
    n = vec3(n.x, n.z, n.y);
    return n;
}

vec3 GetNormalMap(mat3 TBN, vec3 n){
    n = n * 2.0 - 1.0;
    //n.xy *= normalStrength;
    n = normalize(n);
    return normalize(TBN * n);
}

uniform vec2 heightmapTilling = vec2(1, 1);
uniform vec2 heightmapOffset = vec2(0, 0);

void main(){
    vec4 base = texture(mainTex, fsIn.texCoord + uvOffset);
    base = base * color;
    base = color;

    float height = texture(heightMap, fsIn.texCoord + fsIn.uvOffset_).r * 1;
    //base = vec4(height, height, height, 1);
    //base = vec4((fsIn.texCoord + uvOffset), 0, 1);

    Surface surface;
    surface.position = fsIn.worldPos;
    surface.normal = normalize(fsIn.worldNormal);
    //surface.normal = NormalStrength(HeightToNormal(height * heightScale, fsIn.worldNormal, fsIn.worldPos), 1);
    surface.normal = NormalStrength(filterNormalLod(fsIn.texCoord * heightmapTilling + heightmapOffset), 1);
    //surface.normal = NormalStrength(GetNormal(fsIn.texCoord + uvOffset), 1);
    //surface.normal = NormalStrength(GetNormal2(fsIn.texCoord + uvOffset, 1), 1);
    //surface.normal = NormalStrength(_getNormal(fsIn.pos, 1), 1);
    //surface.normal = GetNormalMap(fsIn.TBN, toNormalmap(GetNormal2(fsIn.texCoord + uvOffset, 1)));
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