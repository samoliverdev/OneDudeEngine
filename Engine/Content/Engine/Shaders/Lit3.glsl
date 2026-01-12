#pragma BeginProperties
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

uniform int perDrawInt_1;

#if defined(VERTEX) && defined(MainPass)
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
        mat4 targetModelMatrix = GetModelMatrix();
        vec3 T = normalize(vec3(targetModelMatrix * vec4(tangents, 0.0)));
        vec3 B = normalize(vec3(targetModelMatrix * vec4(cross(tangents, normal), 0.0)));
        vec3 N = normalize(vec3(targetModelMatrix * vec4(normal, 0.0)));

        outPos = localPos.xyz;// pos;
        outNormal = normal;
        outTexCoord = texCoord;
        
        //outTBN = mat3(T, B, N);
        outT = T;
        outB = B;
        outN = N;

        outWorldPos = vec3(targetModelMatrix * localPos); //vec3(targetModelMatrix * vec4(pos, 1.0));
        //vsOut.worldNormal = vec3(targetModelMatrix * vec4(normal, 0));
        outWorldNormal = mat3(transpose(inverse(targetModelMatrix))) * normal; // for non-uniform scale objects

        OutPosition = projection * view * targetModelMatrix * localPos;//GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    #include Engine/ShaderLibrary/Core.glsl
    #include Engine/ShaderLibrary/Common.glsl
    #include Engine/ShaderLibrary/Surface.glsl
    #include Engine/ShaderLibrary/Shadows.glsl
    #include Engine/ShaderLibrary/Light.glsl


    #include Engine/ShaderLibrary/BaseLighting.glsl

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
        /*Out(9) vec4 gAlbedoSpec;
        Out(1) vec3 gPosition;
        Out(2) vec3 gNormal;
        Out(3) vec3 gEmission;
        Out(4) vec3 gOther;*/

        //layout(location = 0) out vec3 gPosition;
        layout(location = 0) out vec3 gNormal;
        layout(location = 1) out vec4 gAlbedoSpec;
        layout(location = 2) out vec4 gOther;

    #else
        Out(0) vec4 fragColor;
    #endif

    const float PI = 3.14159265359;

    vec3 AmbientLight(Surface s){
        return _AmbientLight.rgb * s.color * s.occlusion;
        //return _AmbientLight.rgb * (s.color / PI) * s.occlusion;
    }
    
    vec3 IncomingLight(Surface s, Light l){
        

        ///*
        float NdotL = max(dot(s.normal, l.direction), 0.0);
        return s.color * l.color * l.attenuation * NdotL;
        //return (s.color / PI) * l.color * l.attenuation * NdotL;
        //*/

        /*
        vec3 H = normalize(s.viewDirection + l.direction);
        float diff = max(dot(s.normal, l.direction), 0.0);
        float spec = pow(max(dot(s.normal, H), 0.0), 32.0);
        return (s.color * diff + spec) * l.color * l.attenuation;
        */

        /*
        // ---- Half-Lambert diffuse ----
        float NdotL = dot(s.normal, l.direction);
        float halfLambert = NdotL * 0.5 + 0.5;
        halfLambert = max(halfLambert, 0.0);
        halfLambert *= halfLambert; // Valve-style curve
        // ---- Smoothness → specular power ----
        float specPower = exp2(s.smoothness * 10.0 + 1.0);
        // ---- Blinn-Phong specular ----
        vec3 H = normalize(s.viewDirection + l.direction);
        float spec = pow(max(dot(s.normal, H), 0.0), specPower);
        // ---- Combine ----
        vec3 diffuse = s.color * halfLambert;
        vec3 specular = vec3(0);// vec3(spec); // white spec like HL2
        return (diffuse + specular) * l.color * l.attenuation;
        */
    }

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
        
        //gPosition = surface.position;
        gNormal = vec3(pack_normal_octahedron(surface.normal), 0); //surface.normal;
        gAlbedoSpec.rgb = surface.color.rgb + GetEmission(uv);
        //gAlbedoSpec.a = perDrawInt_1;
        //gAlbedoSpec.a = surface.smoothness;
        //gEmission.rgb = GetEmission(uv);
        gOther = vec4(
            surface.smoothness,
            surface.metallic,
            surface.occlusion,
            perDrawInt_1
        );
        
        #else

        surface.smoothness = clamp(1.0 - smoothness, 0.05, 1);
        vec3 color = GetFinalColor(surface);
        //color += GetEmission(uv);
        fragColor = vec4(color, surface.alpha);
        
        #endif
    }
#endif