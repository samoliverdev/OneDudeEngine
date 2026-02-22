#pragma BeginProperties
    Texture2D mainTex White
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
#pragma EndPassDef

#pragma BeginPassDef
    Name DebugPass
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Core.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
    Uniform vec2 giSize;
    Uniform vec2 screenSize;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)
Texture2D(0, 1, gAlbedoSpec, gAlbedoSpecSampler)
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

    #if defined(MainPass)
    void main(){
        vec4 directLighting = SampleTexture2D(mainTex, mainSampler, texCoord);
        vec2 giUV = texCoord * (giSize / screenSize);
        vec4 giAO = SampleTexture2D(giAO, giAOSampler, giUV);
        vec4 diffuse = texture(gAlbedoSpec, texCoord);
        fragColor = vec4((directLighting.rgb * giAO.a) + (diffuse.rgb * giAO.rgb), directLighting.a);
    }
    #endif

    #if defined(DebugPass)
    void main(){
        vec2 giUV = texCoord * (giSize / screenSize);
        vec4 giAO = SampleTexture2D(giAO, giAOSampler, giUV);
        fragColor = vec4(giAO.rgb, 1.0);
    }
    #endif
#endif
