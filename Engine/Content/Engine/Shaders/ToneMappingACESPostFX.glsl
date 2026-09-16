#pragma BeginPassDef
    Name MainPass
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform float option;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)

#if defined(VERTEX) && defined(MainPass)
    Out(0) vec2 _texCoord;

    void main() {
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    //uniform sampler2D mainTex;

    In(1) vec2 _texCoord;
    Out(0) vec4 fragColor;

    //Source: https://github.com/dmnsgn/glsl-tone-map/blob/main/aces.glsl
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    vec3 aces(vec3 x){
        const float a = 2.51;
        const float b = 0.03;
        const float c = 2.43;
        const float d = 0.59;
        const float e = 0.14;
        return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    }

    float aces(float x){
        const float a = 2.51;
        const float b = 0.03;
        const float c = 2.43;
        const float d = 0.59;
        const float e = 0.14;
        return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    }

    void main() {
        //const float gamma = 2.2;
        vec3 color = texture(mainTex, _texCoord).rgb;
        color.rgb = min(color.rgb, 60.0);
        color = aces(color);
        
        fragColor = vec4(color, 1.0);
    }
#endif