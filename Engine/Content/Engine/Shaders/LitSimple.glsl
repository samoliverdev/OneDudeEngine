#pragma BeginProperties
    Color4 color
    Vector4 sizeOffset 1 1 0 0
    Texture2D mainTex White
    Float metallic 0 0 1
    Float smoothness 0.5 0.0 1.0
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
    Uniform vec3 viewPos;
    Uniform float metallic;
    Uniform float smoothness;
EndUniform()
Texture2D(0, 6, mainTex, mainTexSampler)

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
    #include Engine/ShaderLibrary/PBR.glsl
    
    #include Engine/ShaderLibrary/SurfacePipeline.glsl

    vec3 SurfaceLigthing(Surface surface){
        return GetFinalColor(surface);
    }

    SurfaceOutput SurfaceFunction(SurfaceInput IN){
        SurfaceOutput s = DefaultSurface();

        vec2 uv = IN.uv * sizeOffset.xy + sizeOffset.zw;
        vec4 base = ToLinear(SampleTexture2D(mainTex, mainTexSampler, uv));

        base *= color;

        s.albedo = base.rgb;
        s.alpha = base.a;
        s.normal = ApplyNormalMap(IN.TBN, vec3(0.5, 0.5, 1.0), 1);
        s.metallic = metallic;
        s.smoothness = smoothness;
        
        return s;
    }
#endif