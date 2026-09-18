#pragma BeginPassDef
    Name MainPass
    CullFace NONE
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
    DepthMask True
    DepthTest DISABLE
    RenderPass Forward
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)

#if defined(MainPass)
    #if defined(VERTEX)
    Out(0) vec2 _texCoord;

    void main() {
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
    #endif

    #if defined(FRAGMENT)
    In(0) vec2 _texCoord;
    Out(0) vec4 fragColor;

    void main(){
        vec4 texColor = texture(mainTex, _texCoord);
        //if(texColor.a < 0.1) discard;
        //fragColor = texColor * vec4(color.rgb, 1.0);
        
        fragColor = texColor * color.rgba;
    }
    #endif
#endif