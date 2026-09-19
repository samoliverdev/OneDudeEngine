BeginProperties
    Texture2D mainTex White
EndProperties

BeginPass
    #pragma Name MainPass
    #pragma RenderPass DefaultWindows PostProssing Editor

    #include Engine/ShaderLibrary/Base.glsl
    #include Engine/ShaderLibrary/Core.glsl
    #include Engine/ShaderLibrary/Vertex.glsl

    BeginUniform(0, 0, Main)
        Uniform vec4 color;
    EndUniform()
    Texture2D(0, 1, mainTex, mainSampler)

    BeginVertex
    /*In(0) vec3 vPos;
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
    }*/
    Out(0) vec2 _texCoord;

    void main() {
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = GetLocalPos();
    }
    EndVertex

    BeginFrag
    In(0) vec2 _texCoord;
    Out(0) vec4 fragColor;

    void main() {
        fragColor = SampleTexture2D(mainTex, mainSampler, _texCoord);
    }
    EndFrag
EndPass
