#pragma BeginPassDef
    Name MainPass
    CullFace NONE
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
    DepthMask True
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)

#if defined(MainPass)
    #if defined(VERTEX)
    layout(location = 0) in vec3 _pos;
    layout(location = 1) in vec2 _texCoord;
    out vec2 texCoord;

    BeginUniform(2, 0, CamDraw)
        Uniform mat4 projection;
        Uniform mat4 view;
    EndUniform()

    uniform mat4 model;

    void main(){
        texCoord = _texCoord;
        gl_Position = projection * view * model * vec4(_pos, 1.0);
    }
    #endif

    #if defined(FRAGMENT)
    in vec2 texCoord;
    out vec4 fragColor;

    void main(){
        vec4 texColor = texture(mainTex, texCoord);
        //if(texColor.a < 0.1) discard;
        fragColor = texColor * vec4(color.rgb, 1.0);
    }
    #endif
#endif