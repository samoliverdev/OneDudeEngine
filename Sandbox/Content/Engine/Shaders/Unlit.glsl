#pragma BeginProperties
    Color4 color
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    MultiCompile _ SKINNED INSTANCING
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)
//Texture2D(0, 2, main2Tex, main2Sampler)

layout(set = 3, binding = 0) uniform texture2D matTexs[100];
layout(set = 3, binding = 1) uniform sampler matSamplers[100];

//"layout(set = 0, binding = 0) uniform texture2D[] texs;" or
/*"layout(set = 0, binding = 0) uniform texture2D tex1;
layout(set = 0, binding = 1) uniform texture2D tex2;
..."*/
//#define DefTex(name, bind) const int name = bind;

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
        vec4 texColor = ToSRGB(SampleTexture2D(mainTex, mainSampler, _texCoord)); //texture(sampler2D(mainTex, mainTexSampler), _texCoord); //vec4(_texCoord.xy, 0, 1);// textureSRGB(mainTex, mainTexSampler, _texCoord);
        if(texColor.a < 0.1) discard;
        fragColor = texColor * color;
    }
#endif