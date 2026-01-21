BeginProperties
    Texture2D mainTex White
EndProperties

BeginPass
    #pragma Name MainPass

    #include Engine/ShaderLibrary/Base.glsl
    #include Engine/ShaderLibrary/Core.glsl

    BeginUniform(0, 0, Main)
        Uniform vec4 color;
        Uniform vec2 giSize;
        Uniform vec2 screenSize;
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

    void main(){
        vec3 base = SampleTexture2D(mainTex, mainSampler, texCoord).rgb;
        vec2 scale = giSize / screenSize;
        vec2 giUV = texCoord * scale;
        vec4 giAOData = SampleTexture2D(giAO, giAOSampler, giUV);
        vec3 gi = giAOData.rgb;

        vec3 giEnergy = gi;
        //giEnergy = min(giEnergy, base * 0.8);

        vec3 color = base + giEnergy;
        //fragColor = vec4(color, 1.0);
        //fragColor = vec4(vec3(ao), 1.0);
        fragColor = vec4(gi, 1.0);

        /*vec4 color = SampleTexture2D(mainTex, mainSampler, texCoord); //texture(mainTex, texCoord);
        vec4 giAO = SampleTexture2D(giAO, giAOSampler, texCoord); //texture(mainTex, texCoord);
        //fragColor = vec4(color.rgb + giAO.rgb, 1);
        fragColor = vec4(giAO.rgb, 1);*/
    }
    EndFrag
EndPass
