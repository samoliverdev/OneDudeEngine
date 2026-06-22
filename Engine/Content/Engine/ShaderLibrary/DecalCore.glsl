#ifndef DECAL_CORE_GLSL
#define DECAL_CORE_GLSL

struct DecalInput {
    vec2 screenUV;
    vec3 worldPos;
    vec3 objWorldPos;
    vec3 localPos;
    vec3 surfaceNormalWS;
    vec3 decalNormalWS;

    vec4 gAlbedo;
    vec4 gOther;
};

struct DecalOutput {
    vec3 albedo;
    float alpha;

    float smoothness;
    float metallic;

    bool discardPixel;
};

DecalOutput DefaultDecal(DecalInput IN) {
    DecalOutput d;

    d.albedo = IN.gAlbedo.rgb;
    d.alpha = 1.0;

    d.smoothness = IN.gOther.r;
    d.metallic = IN.gOther.g;

    d.discardPixel = false;
    return d;
}

#endif