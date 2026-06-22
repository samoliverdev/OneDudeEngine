#ifndef DECAL_PIPELINE_GLSL
#define DECAL_PIPELINE_GLSL

flat in vec4 vDecalInvRow0;
flat in vec4 vDecalInvRow1;
flat in vec4 vDecalInvRow2;
flat in vec4 vDecalInvRow3;

in vec3 decalNormalWS;
in vec3 objWorldPos;

layout(location = 1) out vec4 gAlbedoOut;
layout(location = 2) out vec4 gOtherOut;

DecalOutput DecalFunction(DecalInput IN);

void main(){
    mat4 decalWorldToLocal = mat4(
        vDecalInvRow0,
        vDecalInvRow1,
        vDecalInvRow2,
        vDecalInvRow3
    );

    vec2 screenUV = gl_FragCoord.xy / vec2(textureSize(gAlbedoSpec, 0));

    float rawDepth = texture(gDepth, screenUV).r;
    vec3 worldPos = reconstructWorldPos(screenUV, rawDepth, invProjection, invView);

    vec3 localPos = (decalWorldToLocal * vec4(worldPos, 1.0)).xyz;

    if(any(greaterThan(abs(localPos), vec3(0.5))))
        discard;

    vec4 other = texture(gOther, screenUV);

    if(perDrawInt_1 >= 0 && perDrawInt_1 != int(other.a))
        discard;

    vec3 surfaceNormalWS = unpack_normal_octahedron(texture(gNormal, screenUV).rg);

    if(dot(decalNormalWS, surfaceNormalWS) <= 0.0)
        discard;

    DecalInput IN;
    IN.screenUV = screenUV;
    IN.worldPos = worldPos;
    IN.objWorldPos = objWorldPos;
    IN.localPos = localPos;
    IN.surfaceNormalWS = surfaceNormalWS;
    IN.decalNormalWS = decalNormalWS;
    IN.gAlbedo = texture(gAlbedoSpec, screenUV);
    IN.gOther = other;

    DecalOutput d = DecalFunction(IN);

    if(d.discardPixel)
        discard;

    float blendFactor = clamp(d.alpha, 0.0, 1.0);

    #ifdef MANUAL_BLEND
        gAlbedoOut = vec4(
            mix(IN.gAlbedo.rgb, d.albedo, blendFactor),
            blendFactor
        );
        gOtherOut = vec4(
            mix(IN.gOther.r, d.smoothness, blendFactor),
            mix(IN.gOther.g, d.metallic, blendFactor),
            IN.gOther.b,
            IN.gOther.a
        );
    #else
        gAlbedoOut = vec4(d.albedo, blendFactor);
        gOtherOut = vec4(
            d.smoothness,
            d.metallic,
            IN.gOther.b,
            blendFactor
        );
    #endif
}

#endif