#pragma BeginPassDef
    Name MainPass
    Blend ONE ONE
    DepthMask False
    DepthTest ALWAYS
    CullFace FRONT
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/UniformsDef.glsl
#include Engine/ShaderLibrary/TexturesDef.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform int lightIndex;
    Uniform float screenWidth; 
    Uniform float screenHeight;
EndUniform()

Texture2D(0, 6, gPosition, gPositionSampler)
Texture2D(0, 7, gNormal, gNormalSampler)
Texture2D(0, 8, gAlbedoSpec, gAlbedoSpecSampler)
Texture2D(0, 9, gEmission, gEmissionSampler)
Texture2D(0, 10, gOther, gOtherSampler)
Texture2D(0, 10, gDepth, gDepthSampler)
Texture2D(0, 10, sss, sssSampler)

#if defined(VERTEX) && defined(MainPass)
    void main() {
        mat4 targetModelMatrix = GetModelMatrix();
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    Out(0) vec4 FragColor;

    #include Engine/ShaderLibrary/Core.glsl
    #include Engine/ShaderLibrary/Common.glsl
    #include Engine/ShaderLibrary/Surface.glsl
    #include Engine/ShaderLibrary/Shadows.glsl
    #include Engine/ShaderLibrary/Light.glsl
    
    #include Engine/ShaderLibrary/PBR.glsl

    void main(){
        vec2 screenUV = gl_FragCoord.xy / vec2(screenWidth, screenHeight);

        vec3 FragPos = reconstructWorldPos(screenUV, texture(gDepth, screenUV).r, invProjection, invView);

        // retrieve data from G-buffer
        //vec3 FragPos = texture(gPosition, screenUV).rgb;
        vec3 Normal =  unpack_normal_octahedron(texture(gNormal, screenUV).rg); //texture(gNormal, screenUV).rgb;
        vec3 Albedo = texture(gAlbedoSpec, screenUV).rgb;
        vec3 Emission = texture(gEmission, screenUV).rgb;
        float Specular = texture(gOther, screenUV).r;
        float Metallic = texture(gOther, screenUV).g;
        float AO = texture(gOther, screenUV).b;

        vec3 viewPos = invView[3].xyz;

        Surface surface;
        surface.position = FragPos;
        surface.normal = Normal;
        surface.viewDirection = normalize(viewPos - FragPos);
        surface.depth = -(view * vec4(FragPos, 1.0)).z;
        surface.color = Albedo.rgb;
        surface.alpha = 1;
        surface.occlusion = AO;
        surface.metallic = Metallic;
        surface.smoothness = Specular;
        surface.roughness = clamp(1.0 - surface.smoothness, 0.05, 1);

        surface.clearCoat = texture(gNormal, screenUV).b * dot(surface.normal, vec3(0, 1, 0)); //1;
        surface.clearCoatRoughness = 0.05; //0.05;
        surface.clearCoatIOR = 1.5;

        //BRDF brdf = GetBRDF(surface);
        //GI gi = GetGI(surface, brdf);

        ShadowData shadowData = GetShadowData(surface);
        Light light = GetOtherLight(lightIndex, surface, shadowData);

        float sss = texture(sss, screenUV).r;
        //light.attenuation = min(light.attenuation, sss);
        
        vec3 color = IncomingLight3(surface, light);
        //color += Emission;
        FragColor = vec4(color, 1); //surface.alpha);
        //FragColor = vec4(light.color * light.attenuation, 1.0);

        //FragColor = vec4(1.0, 0.0, 1.0, 1.0); // bright magenta
    }
#endif