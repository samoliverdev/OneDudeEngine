#pragma BeginProperties 
    Texture2D mainTex White 
#pragma EndProperties 

#pragma BeginPassDef
    Name Pass1
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name Pass2
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Core.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler) 
Texture2D(0, 3, gDepth, gDepthSampler)
Texture2D(0, 2, giAO, giAOSampler)

#if defined(VERTEX)
    In(0) vec3 vPos;
    In(1) vec2 vTexCoord;
    Out(0) vec3 pos;
    Out(1) vec2 texCoord;

    void main(){
        pos = vPos;
        #if defined(WebGPU_API)
        texCoord = vec2(vTexCoord.x, 1.0 - vTexCoord.y);
        #else
        texCoord = vTexCoord;
        #endif
        OutPosition = vec4(pos, 1.0);
    }
#endif

#if defined(FRAGMENT)
    In(0) vec3 pos;
    In(1) vec2 texCoord;
    Out(0) vec4 fragColor;

    #if defined(Pass1)
    void main(){
        fragColor = SampleTexture2D(giAO, giAOSampler, texCoord);
    }
    #endif

    #if defined(Pass2)
    void main(){
        fragColor = SampleTexture2D(gDepth, gDepthSampler, texCoord);
    }
    #endif
#endif
