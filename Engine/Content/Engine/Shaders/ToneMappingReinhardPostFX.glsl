#pragma BeginPassDef
    Name MainPass
    DepthTest DISABLE
    RenderPass PostProssing
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform float exposure;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)

#if defined(VERTEX) && defined(MainPass)
    Out(0) vec2 _texCoord;

    void main() {
        _texCoord = texCoord.xy;
        OutPosition = GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    //uniform sampler2D mainTex;
    //uniform float exposure;

    In(0) vec2 _texCoord;
    Out(0) vec4 fragColor;

    void main() {
        const float gamma = 2.2;
        vec3 color = texture(mainTex, _texCoord).rgb;
        fragColor = vec4(color, 1.0);
        
        //color.rgb = min(color.rgb, 60.0);
        //color /= (color + vec3(1.0));

        //fragColor.rgb = fragColor.rgb / (fragColor.rgb + vec3(1.0)); // exposure tone mapping
        fragColor.rgb = vec3(1.0) - exp(-fragColor.rgb * exposure); // exposure tone mapping
    }
#endif