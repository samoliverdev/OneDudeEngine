#pragma BeginPassDef
    Name MainPass
    DepthTest DISABLE
    RenderPass PostProssing
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
        _texCoord = texCoord.xy;
        OutPosition = GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    In(0) vec2 _texCoord;
    Out(0) vec4 fragColor;

    //
    // Neutral tonemapping (Hable/Hejl/Frostbite)
    // Input is linear RGB
    //
    vec3 NeutralCurve(vec3 x, float a, float b, float c, float d, float e, float f){
        return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
    }

    float NeutralCurve(float x, float a, float b, float c, float d, float e, float f){
        return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
    }

    vec3 NeutralTonemap(vec3 x){
        // Tonemap
        float a = 0.2;
        float b = 0.29;
        float c = 0.24;
        float d = 0.272;
        float e = 0.02;
        float f = 0.3;
        float whiteLevel = 5.3;
        float whiteClip = 1.0;

        vec3 whiteScale = vec3(1.0) / NeutralCurve(whiteLevel, a, b, c, d, e, f);
        x = NeutralCurve(x * whiteScale, a, b, c, d, e, f);
        x *= whiteScale;

        // Post-curve white point adjustment
        x /= vec3(whiteClip);

        return x;
    }

    void main() {
        //const float gamma = 2.2;
        vec3 color = texture(mainTex, _texCoord).rgb;
        color.rgb = min(color.rgb, 60.0);
        color = NeutralTonemap(color);
        
        fragColor = vec4(color, 1.0);
    }
#endif