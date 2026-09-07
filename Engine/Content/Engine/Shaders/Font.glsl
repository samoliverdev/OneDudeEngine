#pragma BeginProperties
    Color4 color
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    CullFace NONE
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
    DepthMask True
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)

#if defined(MainPass)
    #if defined(VERTEX)
    //layout(location = 0) in vec3 _pos;
    //layout(location = 1) in vec2 _texCoord;
    Out(0) vec2 _texCoord;

    //uniform mat4 model;

    void main(){
        _texCoord = texCoord;
        mat4 targetModelMatrix = GetModelMatrix();
        gl_Position = projection * view * model *  GetLocalPos(); //vec4(_pos, 1.0);
    }
    #endif

    #if defined(FRAGMENT)
    In(0) vec2 _texCoord;
    Out(0) vec4 fragColor;

    void main(){
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(mainTex, _texCoord).r);
        fragColor = vec4(color.rgb, 1.0) * sampled;
        //fragColor = vec4(1, 1, 1, 1) * sampled;
    }
    #endif
#endif