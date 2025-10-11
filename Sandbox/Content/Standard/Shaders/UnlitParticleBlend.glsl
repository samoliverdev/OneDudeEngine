#pragma BeginProperties
    Color4 color
    Texture2D mainTex White
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    DrawType INSTANCING 

    CullFace NONE
    DepthTest NONE
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
    DepthMask False

#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Standard/Shaders/BaseParticle.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)
//Texture2D(0, 2, main2Tex, main2Sampler)

#if defined(VERTEX) && defined(MainPass)
    Out(0) vec2 _texCoord;
    Out(1) vec4 outColor;

    void main(){
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
        outColor = GetColor();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    #include Engine/ShaderLibrary/Core.glsl

    In(0) vec2 _texCoord;
    In(1) vec4 outColor;

    Out(0) vec4 fragColor;

    void main(){
        //vec4 texColor = ToLinear(SampleTexture2D(mainTex, mainSampler, _texCoord)); 
        //fragColor = texColor * outColor; //* color;

        fragColor = outColor; //* color;
    }
#endif