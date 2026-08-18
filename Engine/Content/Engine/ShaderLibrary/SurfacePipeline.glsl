#ifndef SURFACE_PIPELINE
#define SURFACE_PIPELINE

In(0) vec2 outUV;
In(1) vec3 outWorldPos;
In(2) vec3 outWorldNormal;
In(3) vec3 outT;
In(4) vec3 outB;
In(5) vec3 outN;
In(6) vec4 perInstanceDataOut;

#ifdef Deferred
layout(location = 0) out vec3 gNormal;
layout(location = 1) out vec4 gAlbedoSpec;
layout(location = 2) out vec4 gOther;
layout(location = 3) out vec3 gEmission;
#else
Out(0) vec4 fragColor;
#endif

uniform int perDrawInt_1;

Surface ConvertToEngineSurface(SurfaceOutput s, SurfaceInput IN){
    Surface surface;

    surface.position = IN.worldPos;
    surface.normal = normalize(s.normal);
    surface.viewDirection = normalize(IN.viewDir);
    surface.depth = -(view * vec4(IN.worldPos, 1)).z;

    surface.color = s.albedo;
    surface.alpha = s.alpha;

    surface.occlusion = s.occlusion;
    surface.metallic = s.metallic;
    surface.roughness = s.roughness;

    surface.clearCoat = 0;
    surface.clearCoatRoughness = 0;

    return surface;
}

SurfaceInput BuildInput(){
    SurfaceInput IN;

    IN.uv = outUV;
    IN.worldPos = outWorldPos;
    IN.worldNormal = normalize(outWorldNormal);
    IN.viewDir = normalize(viewPos - outWorldPos); //TODO: The User need #define viewPos, Make a better way to Handle This
    IN.TBN = mat3(normalize(outT), normalize(outB), normalize(outN));

    return IN;
}

SurfaceOutput SurfaceFunction(SurfaceInput IN);
vec3 SurfaceLigthing(Surface surface);

void main(){
    SurfaceInput IN = BuildInput();
    SurfaceOutput surf = SurfaceFunction(IN);
    Surface surface = ConvertToEngineSurface(surf, IN);

    #ifdef Deferred
        gNormal = vec3(pack_normal_octahedron(surface.normal), 0.0);
        gAlbedoSpec = vec4(surface.color, 1.0);
        gOther = vec4(surface.roughness, surface.metallic, surface.occlusion, perInstanceDataOut.w); // float(perDrawInt_1));
        gEmission = surf.emission;
    #else
        //BRDF brdf = GetBRDF(surface);
        //GI gi = GetGI(surface, brdf);

        vec3 color = SurfaceLigthing(surface);// GetLighting(surface, brdf, gi);
        color += surf.emission;

        fragColor = vec4(color, surface.alpha);
    #endif
}

#endif