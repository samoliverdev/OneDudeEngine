#pragma BeginPassDef
    Name MainPass
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

#if defined(VERTEX) && defined(MainPass)
    layout (location = 0) in vec3 _pos;
    layout (location = 1) in vec2 _texCoord;

    out vec3 pos;
    out vec2 texCoord;

    void main() {
        pos = _pos;
        texCoord = _texCoord;
        gl_Position = vec4(pos, 1.0);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    uniform sampler2D gPosition;
    uniform sampler2D gNormal;
    uniform sampler2D gAlbedoSpec;
    uniform sampler2D gEmission;
    uniform sampler2D gOther;

    //in vec3 pos;
    in vec2 texCoord;
    out vec4 FragColor;

    uniform vec3 viewPos;
    uniform mat4 view;

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
        vec3 color = GetLighting(surface, brdf, gi);
        color += Emission; //GetEmission(uv);
        FragColor = vec4(color, surface.alpha);
    }
#endif