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
    Out(0) vec2 _texCoord;

    void main(){
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = GetLocalPos();

        #if defined(DefaultWindows) && defined(Vulkan_API)
	    gl_Position.y = -gl_Position.y;	 
        #endif
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
