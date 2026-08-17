#pragma BeginPassDef
    Name MainPass
    Blend ONE ONE
    DepthMask False
    DepthTest ALWAYS
    MultiCompile _ INDIRECTPLUSDIRECTIONAL INDIRECT DIRECTIONAL
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
    Uniform mat4 invProjection;
    Uniform mat4 invView;
EndUniform()

#include Engine/ShaderLibrary/UniformsDef.glsl
#include Engine/ShaderLibrary/TexturesDef.glsl

BeginUniform(0, 0, Main)
    Uniform int lightIndex;
EndUniform()

Texture2D(0, 6, gPosition, gPositionSampler)
Texture2D(0, 7, gNormal, gNormalSampler)
Texture2D(0, 8, gAlbedoSpec, gAlbedoSpecSampler)
Texture2D(0, 9, gEmission, gEmissionSampler)
Texture2D(0, 10, gOther, gOtherSampler)
Texture2D(0, 10, gDepth, gDepthSampler)
Texture2D(0, 10, sss, sssSampler)

#if defined(VERTEX) && defined(MainPass)
    In(0) vec3 vPos;
    In(1) vec2 vTexCoord;
    Out(0) vec3 pos;
    Out(1) vec2 texCoord;

    void main() {
        pos = vPos;
        #if defined(WebGPU_API)
        texCoord = vec2(vTexCoord.x, 1.0 - vTexCoord.y);
        #else
        texCoord = vTexCoord;
        #endif
        OutPosition = vec4(pos, 1.0);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    In(0) vec3 pos;
    In(1) vec2 texCoord;
    Out(0) vec4 FragColor;

    #include Engine/ShaderLibrary/Core.glsl
    #include Engine/ShaderLibrary/Common.glsl
    #include Engine/ShaderLibrary/Surface.glsl
    #include Engine/ShaderLibrary/Shadows.glsl
    #include Engine/ShaderLibrary/Light.glsl
    
    #include Engine/ShaderLibrary/PBR.glsl

    void main(){
        //FragColor = vec4(texture(gAlbedoSpec, texCoord).aaa / 10, 1.0);
        //return;

        float depth = texture(gDepth, texCoord).r;
        
        if(depth >= 1.0) discard;


        vec3 FragPos = reconstructWorldPos(texCoord, depth, invProjection, invView);

        //FragColor = vec4(FragPos.xyz, 1);
        //FragColor = vec4(texture(gPosition, texCoord).rgb, 1);
        //return; 

        // retrieve data from G-buffer
        //vec3 FragPos = texture(gPosition, texCoord).rgb;
        vec3 Normal = unpack_normal_octahedron(texture(gNormal, texCoord).rg); //texture(gNormal, texCoord).rgb;
        vec3 Albedo = texture(gAlbedoSpec, texCoord).rgb;
        vec3 Emission = texture(gEmission, texCoord).rgb;
        float Specular = texture(gOther, texCoord).r;
        float Metallic = texture(gOther, texCoord).g;
        float AO = texture(gOther, texCoord).b;

        vec3 viewPos = invView[3].xyz;

        Surface surface;
        surface.position = FragPos;
        surface.normal = Normal;
        surface.viewDirection = normalize(viewPos - FragPos);
        surface.depth = -(view * vec4(FragPos, 1.0)).z;
        surface.color = Albedo.rgb;
        surface.alpha = 1.0;
        surface.occlusion = AO;
        surface.metallic = Metallic;
        surface.smoothness = Specular;
        surface.roughness = clamp(1.0 - surface.smoothness, 0.05, 1);

        surface.clearCoat = texture(gNormal, texCoord).b * dot(surface.normal, vec3(0, 1, 0)); //1;
        surface.clearCoatRoughness = 0.75; //0.05; //0.05;
        surface.clearCoatIOR = 1.5;

        //FragColor = vec4(FragPos, 1.0);
        //return;

        //BRDF brdf = GetBRDF(surface);
        //GI gi = GetGI(surface, brdf);

        FragColor = vec4(1, 1, 1, 1);

        //FragColor = vec4(texture(sss, texCoord).rgb, surface.alpha);
        //return;

        float sss = texture(sss, texCoord).r;

        #ifdef INDIRECTPLUSDIRECTIONAL
            vec3 color = AmbientLight3(surface);

            ShadowData shadowData = GetShadowData(surface);
            Light light = GetDirectionalLight(lightIndex, surface, shadowData);
            light.attenuation = min(light.attenuation, sss);

		    color += IncomingLight3(surface, light);
            color += Emission; //TODO: Review this later to check if is right
            FragColor = vec4(color, 1); //surface.alpha);
        #endif

        #if defined(INDIRECT)
	        vec3 color = AmbientLight3(surface);
            color += Emission; //TODO: Review this later to check if is right
            FragColor = vec4(color, 1); //surface.alpha);
        #endif

        #if defined(DIRECTIONAL)
            ShadowData shadowData = GetShadowData(surface);
            Light light = GetDirectionalLight(lightIndex, surface, shadowData);
            //light.attenuation = min(light.attenuation, sss);

		    vec3 color = IncomingLight3(surface, light);
            //color += Emission; //TODO: Review this later to check if is right
            FragColor = vec4(color, 1); //surface.alpha);
        #endif
    }
#endif