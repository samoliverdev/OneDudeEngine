#pragma BeginPassDef
    Name MainPass
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform float exposure;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)

#if defined(VERTEX) && defined(MainPass)
    Out(0) vec2 _texCoord;

    void main() {
        _texCoord = texCoord.xy;
        OutPosition = GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    In(0) vec2 _texCoord;
    Out(0) vec4 fragColor;

    const float offset = 1.0 / 300.0; 

    void main() {
        const float gamma = 2.2;
        vec3 hdrColor = texture(mainTex, _texCoord).rgb;
    
        // reinhard tone mapping
        vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
        // gamma correction 
        //mapped = pow(mapped, vec3(1.0 / gamma));
    
        fragColor = vec4(mapped, 1.0);
    }
#endif