#pragma BeginPassDef
    Name MainPass
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
    Uniform vec3 viewPos;
EndUniform()

Texture2D(0, 6, gPosition, gPositionSampler)
Texture2D(0, 7, gNormal, gNormalSampler)
Texture2D(0, 8, gAlbedoSpec, gAlbedoSpecSampler)
Texture2D(0, 8, gEmission, gEmissionSampler)
Texture2D(0, 10, gOther, gOtherSampler)
Texture2D(0, 10, gDepth, gDepthSampler)

#if defined(VERTEX) && defined(MainPass)
    /*layout(location = 0) in vec3 _pos;
    layout(location = 1) in vec2 _texCoord;
    out vec3 pos;
    out vec2 texCoord;*/

    In(0) vec3 vPos;
    In(1) vec2 vTexCoord;
    Out(0) vec3 pos;
    Out(1) vec2 texCoord;

    void main() {
        /*pos = _pos;
        texCoord = _texCoord;
        gl_Position = vec4(pos, 1.0);*/

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
    /*uniform sampler2D gPosition;
    uniform sampler2D gNormal;
    uniform sampler2D gAlbedoSpec;
    uniform sampler2D gEmission;
    uniform sampler2D gOther;
    uniform vec3 viewPos;
    uniform mat4 view;*/

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
    #include Engine/ShaderLibrary/Fog.glsl

    void main(){
        FragColor = vec4(texture(gAlbedoSpec, texCoord).aaa, 1.0);
        return;

        vec3 FragPos = reconstructWorldPos(texCoord, texture(gDepth, texCoord).r, invProjection, invView);

        // retrieve data from G-buffer
        //vec3 FragPos = texture(gPosition, texCoord).rgb;
        vec3 Normal = unpack_normal_octahedron(texture(gNormal, texCoord).rg); //texture(gNormal, texCoord).rgb;
        vec3 Albedo = texture(gAlbedoSpec, texCoord).rgb;//this is linar
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

        //FragColor = vec4(Normal, 1);
        //return;

        BRDF brdf = GetBRDF(surface);
        GI gi = GetGI(surface, brdf);
        vec3 color = GetLighting(surface, brdf, gi);
        color += Emission;
        FragColor = vec4(color, surface.alpha);

        //FragColor = ApplyFog(FragColor, length(FragPos - viewPos));

    }
#endif