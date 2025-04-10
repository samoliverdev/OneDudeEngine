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
        fragColor = SampleTexture2D(mainTex, mainSampler, texCoord); //texture(mainTex, texCoord);
        //fragColor = vec4(texCoord.xy, 0, 1);
    }
    EndFrag
EndPass
