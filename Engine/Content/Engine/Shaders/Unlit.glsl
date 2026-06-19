#pragma BeginProperties
    Texture2D mainTex White
    Color4 color
    Float intensity 1
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    CullFace BACK
    Tags ForwardOnly
    DrawType _ SKINNED INSTANCING INSTANCINGMATRIX43
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
    Uniform float intensity;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)
//Texture2D(0, 2, main2Tex, main2Sampler)

#if defined(VERTEX) && defined(MainPass)
    Out(0) vec2 _texCoord;

    void main(){
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    #include Engine/ShaderLibrary/Core.glsl

    In(0) vec2 _texCoord;
    Out(0) vec4 fragColor;

    void main(){
        vec4 texColor = ToLinear(SampleTexture2D(mainTex, mainSampler, _texCoord)); 
        if(texColor.a < 0.1) discard;
        fragColor = vec4(texColor.rgb * (color.rgb * intensity), texColor.a * color.a);
    }
#endif