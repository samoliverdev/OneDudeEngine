BeginProperties
    Texture2D mainTex White
EndProperties

BeginPass
    #pragma Name MainPass

    #include Engine/ShaderLibrary/Base.glsl
    #include Engine/ShaderLibrary/Core.glsl

    BeginUniform(0, 0, Main)
        Uniform vec4 color;
    EndUniform()
    Texture2D(0, 1, mainTex, mainSampler)
    Texture2D(0, 2, giAO, giAOSampler)

    BeginVertex
    In(0) vec3 vPos;
    In(1) vec2 vTexCoord;
    Out(0) vec3 pos;
    Out(1) vec2 texCoord;

    void main() {
        pos = vPos;
        #if defined(WebGPU_API)
        texCoord = vec2(vTexCoord.x, 1.0 - vTexCoord.y);
        #else
        texCoord = vTexCoord;
        #endif
        OutPosition = vec4(pos, 1.0);
    }
    EndVertex

    BeginFrag
    In(0) vec3 pos;
    In(1) vec2 texCoord;
    Out(0) vec4 fragColor;

    //uniform sampler2D mainTex;

    void main() {
        //fragColor = vec4(1, 0, 0, 1);
        vec4 color = SampleTexture2D(mainTex, mainSampler, texCoord); //texture(mainTex, texCoord);
        vec4 giAO = SampleTexture2D(giAO, giAOSampler, texCoord); //texture(mainTex, texCoord);
        
        fragColor = vec4(color.rgb + giAO.rgb, 1);
        //fragColor = vec4(giAO.rgb, 1);
    }
    EndFrag
EndPass
