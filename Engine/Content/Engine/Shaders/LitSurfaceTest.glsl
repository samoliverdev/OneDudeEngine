#pragma BeginProperties
    Color4 color
    Vector4 sizeOffset 1 1 0 0
    Texture2D mainTex White
    Texture2D normalMap Normal
    Float normalStrength 1 0 10
    Texture2D emissionMap Black
    Color4 emissionColor 0 0 0 0
    Float emissionIntensity 1 
    Texture2D maskMap White
    Float occlusion 1 0 1
    Float metallic 0 0 1
    Float smoothness 0.5 0.0 1.0
    Float cutoff 0.5 0 1
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    DrawType _ SKINNED INSTANCING INSTANCINGMATRIX43
    MultiCompile Opaque Blend
    MultiCompile Forward Deferred

    CullFace BACK
    DepthTest LESS
    Blend Off
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl
#include Engine/ShaderLibrary/UniformsDef.glsl
#include Engine/ShaderLibrary/TexturesDef.glsl
#include Engine/ShaderLibrary/SurfaceCore.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
    Uniform vec4 sizeOffset;
    Uniform vec4 emissionColor;
    Uniform vec3 viewPos;
    Uniform float normalStrength;
    Uniform float emissionIntensity;
    Uniform float occlusion;
    Uniform float metallic;
    Uniform float smoothness;
    Uniform float cutoff;
EndUniform()
Texture2D(0, 6, mainTex, mainTexSampler)
Texture2D(0, 7, normalMap, normalMapSampler)
Texture2D(0, 8, emissionMap, emissionMapSampler)
Texture2D(0, 9, maskMap, maskMapSampler)

// =====================================================
// VERTEX
// =====================================================
#if defined(VERTEX) && defined(MainPass)
    #include Engine/ShaderLibrary/SurfaceVertex.glsl
#endif

// =====================================================
// FRAGMENT
// =====================================================
#if defined(FRAGMENT) && defined(MainPass)
    #include Engine/ShaderLibrary/Core.glsl
    #include Engine/ShaderLibrary/Common.glsl
    #include Engine/ShaderLibrary/Surface.glsl
    #include Engine/ShaderLibrary/Shadows.glsl
    #include Engine/ShaderLibrary/Light.glsl
    #include Engine/ShaderLibrary/BRDF.glsl
    #include Engine/ShaderLibrary/GI.glsl
    #include Engine/ShaderLibrary/Lighting.glsl
    
    #include Engine/ShaderLibrary/SurfacePipeline.glsl

    SurfaceOutput SurfaceFunction(SurfaceInput IN){
        SurfaceOutput s = DefaultSurface();

        vec2 uv = IN.uv * sizeOffset.xy + sizeOffset.zw;
        vec4 base = ToLinear(SampleTexture2D(mainTex, mainTexSampler, uv));
        if(base.a < cutoff) discard;

        base *= color;

        s.albedo = base.rgb;
        s.alpha = base.a;

        // NORMAL
        vec3 nTex = SampleTexture2D(normalMap, normalMapSampler, uv).xyz;
        s.normal = ApplyNormalMap(IN.TBN, nTex, normalStrength);

        // MASK
        vec4 mask = SampleTexture2D(maskMap, maskMapSampler, uv);

        s.metallic = metallic * mask.r;
        s.smoothness = smoothness * mask.a;
        s.occlusion = mix(mask.g, 1.0, occlusion);

        // EMISSION
        vec3 eTex = ToLinear(SampleTexture2D(emissionMap, emissionMapSampler, uv)).rgb;
        s.emission = eTex * emissionColor.rgb * emissionIntensity;
        
        return s;
    }
#endif