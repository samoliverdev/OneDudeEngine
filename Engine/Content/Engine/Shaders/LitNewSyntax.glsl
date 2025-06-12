BeginProperties
    Color4 color
    Vector4 sizeOffset 1 1 0 0
    Texture2D mainTex White
    Texture2D normalMap Normal
    Float normalStrength 1 0 10
    Texture2D emissionMap Black
    Color4 emissionColor 0 0 0 0
    Texture2D maskMap White
    Float occlusion 1 0 1
    Float metallic 0 0 1
    Float smoothness 0.5 0.0 1.0
    Float cutoff 0.5 0 1
EndProperties

BeginPass
    #pragma Name MainPass
    #pragma SupportInstancing true
    #pragma MultiCompile _ SKINNED INSTANCING
    #pragma MultiCompile Opaque Blend
    #pragma MultiCompile Forward Deferred
    #pragma CullFace BACK
    #pragma DepthTest LESS
    #pragma Blend Off

    #include "Engine/ShaderLibrary/Base.glsl"
    #include "Engine/ShaderLibrary/Vertex.glsl"

    #include Engine/ShaderLibrary/UniformsDef.glsl
    #include Engine/ShaderLibrary/TexturesDef.glsl

    BeginUniform(0, 0, Main)
        Uniform vec3 viewPos;
        Uniform float normalStrength;
        Uniform vec4 color;
        Uniform vec4 sizeOffset;
        Uniform vec4 emissionColor;
        Uniform float occlusion;
        Uniform float metallic;
        Uniform float smoothness;
        Uniform float cutoff;
    EndUniform()

    Texture2D(0, 6, mainTex, mainTexSampler)
    Texture2D(0, 7, normalMap, normalMapSampler)
    Texture2D(0, 8, emissionMap, emissionMapSampler)
    Texture2D(0, 9, maskMap, maskMapSampler)

    BeginVertex
    Out(0) vec3 outPos;
    Out(1) vec3 outNormal;
    Out(2) vec2 outTexCoord;
    Out(3) vec3 outWorldPos;
    Out(4) vec3 outWorldNormal;
    //Out(5) mat3 outTBN;
    Out(5) vec3 outT;
    Out(6) vec3 outB;
    Out(7) vec3 outN;

    void main(){
        vec4 localPos = GetLocalPos();
        vec3 localNormal = GetLocalNormal();
        vec3 localTangents = GetLocalTangent();

        mat4 targetModelMatrix = GetModelMatrix();
        vec3 T = normalize(vec3(targetModelMatrix * vec4(localTangents, 0.0)));
        vec3 B = normalize(vec3(targetModelMatrix * vec4(cross(localTangents, localNormal), 0.0)));
        vec3 N = normalize(vec3(targetModelMatrix * vec4(localNormal, 0.0)));

        outPos = localPos.xyz;// pos;
        outNormal = localNormal;
        outTexCoord = texCoord;
        
        //outTBN = mat3(T, B, N);
        outT = T;
        outB = B;
        outN = N;

        outWorldPos = vec3(targetModelMatrix * localPos); //vec3(targetModelMatrix * vec4(pos, 1.0));
        //vsOut.worldNormal = vec3(targetModelMatrix * vec4(normal, 0));
        outWorldNormal = mat3(transpose(inverse(targetModelMatrix))) * localNormal; // for non-uniform scale objects

        OutPosition = projection * view * targetModelMatrix * localPos;//GetLocalPos();
    }
    EndVertex

    BeginFrag
    #include Engine/ShaderLibrary/Core.glsl
    #include Engine/ShaderLibrary/Common.glsl
    #include Engine/ShaderLibrary/Surface.glsl
    #include Engine/ShaderLibrary/Shadows.glsl
    #include Engine/ShaderLibrary/Light.glsl
    #include Engine/ShaderLibrary/BRDF.glsl
    #include Engine/ShaderLibrary/GI.glsl
    #include Engine/ShaderLibrary/Lighting.glsl

    In(0) vec3 outPos;
    In(1) vec3 outNormal;
    In(2) vec2 outTexCoord;
    In(3) vec3 outWorldPos;
    In(4) vec3 outWorldNormal;
    //In(5) mat3 outTBN;
    In(5) vec3 outT;
    In(6) vec3 outB;
    In(7) vec3 outN;
    
    #ifdef Deferred
        Out(9) vec4 gAlbedoSpec;

        Out(1) vec3 gPosition;
        Out(2) vec3 gNormal;
        //Out(3) vec4 gAlbedoSpec;
        Out(3) vec3 gEmission;
        Out(4) vec3 gOther;
    #else
        Out(0) vec4 fragColor;
    #endif

    vec3 GetEmission(vec2 baseUV){
        vec4 map = SampleTexture2D(emissionMap, emissionMapSampler, baseUV); //texture(emissionMap, baseUV);
        return map.rgb * emissionColor.rgb;
    }

    vec4 GetMask(vec2 baseUV){
        return SampleTexture2D(maskMap, maskMapSampler, baseUV);// texture(maskMap, baseUV);
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
        //return 1.0;

        float strength = occlusion;
        float _occlusion = GetMask(baseUV).g;
        _occlusion = mix(_occlusion, 1.0, strength);
        return _occlusion;
    }

    vec3 GetNormal(mat3 TBN, vec2 uv){
        vec3 n = SampleTexture2D(normalMap, normalMapSampler, uv).xyz; // texture(normal, uv).xyz;
        n = n * 2.0 - 1.0;
        n.xy *= normalStrength;
        n = normalize(n);
        return normalize(TBN * n);
    }

    float Dither(vec2 fragCoord) {
        return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 0.001;
        //return fract(sin(dot(fragCoord, vec2(12.9898, 78.233))) * 43758.5453) * 0.003;
    }

    void main(){
        //vec4 sizeOffset = vec4(1.0, 1.0, 0.0, 0.0);
        vec2 uv = outTexCoord * sizeOffset.xy + sizeOffset.zw;
        vec4 base = ToLinear(SampleTexture2D(mainTex, mainTexSampler, uv)); //textureSRGB(mainTex, uv);
        if(base.a < cutoff) discard;
        base = base * color;
        
        /*vec3 normalMap = exture(normal, uv).rgb);
        vec3 _normal = normalize(normalMap * 2.0 - 1.0); // transforms from [-1,1] to [0,1] 
        _normal = normalize(fsIn.TBN * _normal);*/ 
        
        vec3 _normal = GetNormal(mat3(outT, outB, outN), uv);// GetNormal(outTBN, uv);

        Surface surface;
        surface.position = outWorldPos;
        surface.normal = outWorldNormal;// _normal;
        surface.viewDirection = normalize(viewPos - outWorldPos);
        surface.depth = -(view * vec4(outWorldPos, 1)).z;
        surface.color = base.rgb;
        surface.alpha = base.a;
        surface.occlusion = GetOcclusion(uv);
        surface.metallic = GetMetallic(uv);
        surface.smoothness = GetSmoothness(uv);

        #ifdef Deferred
        
        gPosition = surface.position;
        gNormal = surface.normal;
        gAlbedoSpec.rgb = surface.color.rgb;
        //gAlbedoSpec.a = surface.smoothness;
        gEmission.rgb = GetEmission(uv);
        gOther.r = surface.smoothness;
        gOther.g = surface.metallic;
        gOther.b = surface.occlusion;
        
        #else

        /*vec3 lightDir = normalize(_DirectionalLightDirections[0].xyz);
        float diff = max(dot(_normal, lightDir), 0.0);
        vec3 diffuse = diff * _DirectionalLightColors[0].rgb;
        vec4 texColor = base;
        vec3 ambient = _AmbientLight;// * texColor.rgb;
        fragColor = vec4((diffuse + ambient) * texColor.rgb, texColor.a);
        return;*/

        //fragColor = vec4(surface.normal, 1);
        //return;

        BRDF brdf = GetBRDF(surface);
        GI gi = GetGI(surface, brdf);
        vec3 color = GetLighting(surface, brdf, gi);// + vec3(Dither(gl_FragCoord.xy)); //Fixme: Reduce the Color Banding, Temp fixed
        color += GetEmission(uv);
        fragColor = vec4(color, surface.alpha);
        
        #endif
    }
    EndFrag
EndPass
