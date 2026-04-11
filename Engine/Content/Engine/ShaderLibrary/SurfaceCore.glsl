#ifndef SURFACE_CORE
#define SURFACE_CORE

struct SurfaceInput{
    vec2 uv;
    vec3 worldPos;
    vec3 worldNormal;
    vec3 viewDir;
    mat3 TBN;
};

struct SurfaceOutput{
    vec3 albedo;
    vec3 normal;
    vec3 emission;

    float metallic;
    float smoothness;
    float occlusion;
    float alpha;
};

SurfaceOutput DefaultSurface(){
    SurfaceOutput s;
    s.albedo = vec3(1.0);
    s.normal = vec3(0.0, 0.0, 1.0);
    s.emission = vec3(0.0);
    s.metallic = 0.0;
    s.smoothness = 0.5;
    s.occlusion = 1.0;
    s.alpha = 1.0;
    return s;
}

vec3 UnpackNormal(vec3 n){
    return normalize(n * 2.0 - 1.0);
}

vec3 ApplyNormalMap(mat3 TBN, vec3 normalTex, float strength){
    vec3 n = UnpackNormal(normalTex);
    n.xy *= strength;
    return normalize(TBN * n);
}
#endif