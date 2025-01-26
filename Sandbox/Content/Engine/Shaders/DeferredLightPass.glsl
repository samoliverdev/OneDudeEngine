#pragma BeginPassDef
    Name MainPass
#pragma EndPassDef

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

    //in vec3 pos;
    in vec2 texCoord;
    out vec4 FragColor;

    struct Light {
        vec3 Position;
        vec3 Color;
    };

    const int NR_LIGHTS = 32;
    uniform Light lights[NR_LIGHTS];
    uniform int lightsCount = 0;
    uniform vec3 viewPos;

    void main() {
        // retrieve data from G-buffer
        vec3 FragPos = texture(gPosition, texCoord).rgb;
        vec3 Normal = texture(gNormal, texCoord).rgb;
        vec3 Albedo = texture(gAlbedoSpec, texCoord).rgb;
        float Specular = texture(gAlbedoSpec, texCoord).a;
        
        // then calculate lighting as usual
        vec3 lighting = Albedo * 0.1; // hard-coded ambient component
        vec3 viewDir = normalize(viewPos - FragPos);
        for(int i = 0; i < lightsCount; ++i){
            // diffuse
            vec3 lightDir = normalize(lights[i].Position - FragPos);
            vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * lights[i].Color;
            lighting += diffuse;
        }
        
        FragColor = vec4(lighting, 1.0);
    }
#endif