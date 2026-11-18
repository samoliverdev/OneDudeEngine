#pragma BeginProperties
    Texture2D heightMap Black
    Texture2D paintMap White
    Texture2D albedo0 White
    Texture2D albedo1 White
    Texture2D albedo2 White
    Texture2D albedo3 White
    Texture2D normal0 Normal
    Texture2D normal1 Normal
    Texture2D normal2 Normal
    Texture2D normal3 Normal
#pragma EndProperties
#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    DrawType _ INSTANCING
    MultiCompile Forward Deferred
    RenderPass Forward Deferred DeferredCopy
    CullFace BACK
    DepthTest LESS
    Blend Off
#pragma EndPassDef

#include Engine/Shaders/TerrainSystem/TerrainGeometry.glsl
#include Engine/ShaderLibrary/PipelineDataDef.glsl

#if defined(VERTEX)
Out(0) vec2 terrainUV;
Out(1) vec3 worldPosition;
Out(2) vec3 worldNormal;
Out(3) vec3 worldTangent;
OutFlat(4) float terrainLayer;
void main(){
    terrainUV=TerrainUV();
    vec4 world=TerrainPosition(terrainUV);
    worldPosition=world.xyz;
    vec3 n=TerrainLocalNormal(terrainUV);
    worldNormal=normalize(mat3(terrainNormal)*n);
    worldTangent=normalize(mat3(terrainModel)*vec3(n.y,-n.x,0));
    terrainLayer=GetPerInstanceData().w;
    OutPosition=projection*view*world;
}
#endif

#if defined(FRAGMENT)
#include Engine/ShaderLibrary/Core.glsl
#include Engine/ShaderLibrary/Common.glsl
#include Engine/ShaderLibrary/Surface.glsl
#include Engine/ShaderLibrary/Shadows.glsl
#include Engine/ShaderLibrary/Light.glsl
#include Engine/ShaderLibrary/PBR.glsl
In(0) vec2 terrainUV;
In(1) vec3 worldPosition;
In(2) vec3 worldNormal;
In(3) vec3 worldTangent;
InFlat(4) float terrainLayer;
#ifdef Deferred
layout(location=0) out vec3 gNormal;
layout(location=1) out vec4 gAlbedoSpec;
layout(location=2) out vec4 gOther;
layout(location=3) out vec3 gEmission;
#else
Out(0) vec4 fragColor;
#endif
void main(){
    vec4 weights=max(SampleTexture2D(paintMap,paintMapSampler,terrainUV),vec4(0));
    float total=dot(weights,vec4(1));
    weights=total>0.0001?weights/total:vec4(1,0,0,0);
    vec2 metres=terrainUV*terrainSize.xz;
    vec2 uv0=metres/tileSize0,uv1=metres/tileSize1,uv2=metres/tileSize2,uv3=metres/tileSize3;
    vec3 color=ToLinear(SampleTexture2D(albedo0,albedo0Sampler,uv0)).rgb*tint0.rgb*weights.x
        +ToLinear(SampleTexture2D(albedo1,albedo1Sampler,uv1)).rgb*tint1.rgb*weights.y
        +ToLinear(SampleTexture2D(albedo2,albedo2Sampler,uv2)).rgb*tint2.rgb*weights.z
        +ToLinear(SampleTexture2D(albedo3,albedo3Sampler,uv3)).rgb*tint3.rgb*weights.w;
    vec3 n0=SampleTexture2D(normal0,normal0Sampler,uv0).xyz*2-1;
    vec3 n1=SampleTexture2D(normal1,normal1Sampler,uv1).xyz*2-1;
    vec3 n2=SampleTexture2D(normal2,normal2Sampler,uv2).xyz*2-1;
    vec3 n3=SampleTexture2D(normal3,normal3Sampler,uv3).xyz*2-1;
    n0.xy*=layerNormalStrength.x;n1.xy*=layerNormalStrength.y;
    n2.xy*=layerNormalStrength.z;n3.xy*=layerNormalStrength.w;
    vec3 detail=normalize(n0*weights.x+n1*weights.y+n2*weights.z+n3*weights.w+vec3(0,0,0.00001));
    vec3 N=normalize(worldNormal),T=normalize(worldTangent);
    T=normalize(T-N*dot(N,T));
    // Surface UV +V runs along terrain +Z (opposite cross(N,T)).
    vec3 B=normalize(cross(T,N));
    Surface surface;
    surface.position = worldPosition;
    surface.viewDirection = normalize(invView[3].xyz - worldPosition);

    // Normal-map slopes can face away from the camera at grazing angles.
    // Keep a small positive NdotV so Fresnel and reflections remain well-defined.
    // Limit the correction to the geometric normal's visibility to preserve flat normals.
    vec3 mappedNormal = normalize(mat3(T, B, N) * detail);
    float minNdotV = min(0.02, max(dot(N, surface.viewDirection), 0.0));
    float correction = max(0.0, minNdotV - dot(mappedNormal, surface.viewDirection));
    surface.normal = normalize(mappedNormal + surface.viewDirection * correction);
    surface.depth=-(view*vec4(worldPosition,1)).z;
    surface.color=color;surface.alpha=1;
    surface.occlusion=1;surface.metallic=dot(weights,layerMetallic);
    surface.roughness=clamp(dot(weights,layerRoughness),0.025,1);
    surface.clearCoat=0;surface.clearCoatRoughness=0.1;
#ifdef Deferred
    gNormal=vec3(pack_normal_octahedron(surface.normal),0);
    gAlbedoSpec=vec4(surface.color,0.1);
    gOther=vec4(surface.roughness,surface.metallic,surface.occlusion,terrainLayer);
    gEmission=vec3(0);
#else
    fragColor=vec4(GetFinalColor(surface),1);
#endif
}
#endif
