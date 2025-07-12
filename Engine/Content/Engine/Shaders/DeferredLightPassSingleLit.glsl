#pragma BeginPassDef
    Name MainPass
    Blend ONE ONE
    DepthMask False
    MultiCompile _ INDIRECT DIRECTIONAL OTHER
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
EndUniform()

#include Engine/ShaderLibrary/UniformsDef.glsl
#include Engine/ShaderLibrary/TexturesDef.glsl

BeginUniform(0, 0, Main)
    Uniform vec3 viewPos;
    Uniform int lightIndex;
EndUniform()

Texture2D(0, 6, gPosition, gPositionSampler)
Texture2D(0, 7, gNormal, gNormalSampler)
Texture2D(0, 8, gAlbedoSpec, gAlbedoSpecSampler)
Texture2D(0, 9, gEmission, gEmissionSampler)
Texture2D(0, 10, gOther, gOtherSampler)

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
    #include Engine/ShaderLibrary/BRDF.glsl
    #include Engine/ShaderLibrary/GI.glsl
    #include Engine/ShaderLibrary/Lighting.glsl

    void main(){
        // retrieve data from G-buffer
        vec3 FragPos = texture(gPosition, texCoord).rgb;
        vec3 Normal = texture(gNormal, texCoord).rgb;
        vec3 Albedo = texture(gAlbedoSpec, texCoord).rgb;
        vec3 Emission = texture(gEmission, texCoord).rgb;
        float Specular = texture(gOther, texCoord).r;
        float Metallic = texture(gOther, texCoord).g;
        float AO = texture(gOther, texCoord).b;

        Surface surface;
        surface.position = FragPos;
        surface.normal = Normal;
        surface.viewDirection = normalize(viewPos - FragPos);
        surface.depth = -(view * vec4(FragPos, 1.0)).z;
        surface.color = Albedo.rgb;
        surface.alpha = AO;
        surface.occlusion = 1.0;
        surface.metallic = Metallic;
        surface.smoothness = Specular;

        BRDF brdf = GetBRDF(surface);
        GI gi = GetGI(surface, brdf);

        FragColor = vec4(1,1,1, 1);

        #if defined(INDIRECT)
	        vec3 color = IndirectBRDF(surface, brdf, gi.diffuse, gi.specular);
            FragColor = vec4(color, surface.alpha);
        #endif

        #if defined(DIRECTIONAL)
            ShadowData shadowData = GetShadowData(surface);
            Light light = GetDirectionalLight(lightIndex, surface, shadowData);
		    vec3 color = GetLighting(surface, brdf, light);
            FragColor = vec4(color, surface.alpha);
        #endif

        #if defined(OTHER)
            ShadowData shadowData = GetShadowData(surface);
            Light light = GetOtherLight(lightIndex, surface, shadowData);
		    vec3 color = GetLighting(surface, brdf, light);
            FragColor = vec4(color, surface.alpha);
        #endif
    }
#endif