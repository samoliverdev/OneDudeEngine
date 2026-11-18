#ifndef TERRAIN_SYSTEM_GEOMETRY
#define TERRAIN_SYSTEM_GEOMETRY
#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform vec3 terrainSize;
    Uniform vec2 heightSize;
    Uniform mat4 terrainModel;
    Uniform mat4 terrainNormal;
    Uniform vec2 tileSize0;
    Uniform vec2 tileSize1;
    Uniform vec2 tileSize2;
    Uniform vec2 tileSize3;
    Uniform vec4 tint0;
    Uniform vec4 tint1;
    Uniform vec4 tint2;
    Uniform vec4 tint3;
    Uniform vec4 layerMetallic;
    Uniform vec4 layerRoughness;
    Uniform vec4 layerNormalStrength;
EndUniform()
Texture2D(0, 1, heightMap, heightMapSampler)
Texture2D(0, 2, paintMap, paintMapSampler)
Texture2D(0, 3, albedo0, albedo0Sampler)
Texture2D(0, 4, albedo1, albedo1Sampler)
Texture2D(0, 5, albedo2, albedo2Sampler)
Texture2D(0, 6, albedo3, albedo3Sampler)
Texture2D(0, 7, normal0, normal0Sampler)
Texture2D(0, 8, normal1, normal1Sampler)
Texture2D(0, 9, normal2, normal2Sampler)
Texture2D(0, 10, normal3, normal3Sampler)

// CPU sample (0,0) and (columns-1,rows-1) coincide with the terrain boundaries.
float TerrainHeight(vec2 uv){
    vec2 texelUV=(clamp(uv,vec2(0),vec2(1))*(heightSize-vec2(1))+vec2(0.5))/heightSize;
    return SampleTexture2D(heightMap,heightMapSampler,texelUV).r*terrainSize.y;
}
#if defined(VERTEX)
vec2 TerrainUV(){
    vec4 patchData=GetPerInstanceData();
    return clamp(patchData.xy+pos.xz*patchData.z,vec2(0),vec2(1));
}
vec4 TerrainPosition(vec2 uv){
    return GetModelMatrix()*vec4(pos.x,TerrainHeight(uv),pos.z,1);
}
vec3 TerrainLocalNormal(vec2 uv){
    vec2 du=vec2(1)/(heightSize-vec2(1));
    vec2 lo=max(vec2(0),uv-du),hi=min(vec2(1),uv+du);
    float dx=(TerrainHeight(vec2(hi.x,uv.y))-TerrainHeight(vec2(lo.x,uv.y)))/max((hi.x-lo.x)*terrainSize.x,0.0001);
    float dz=(TerrainHeight(vec2(uv.x,hi.y))-TerrainHeight(vec2(uv.x,lo.y)))/max((hi.y-lo.y)*terrainSize.z,0.0001);
    return normalize(vec3(-dx,1,-dz));
}
#endif
#endif
