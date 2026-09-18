#pragma BeginPassDef
    Name MainPass
    CullFace NONE
    DepthTest LESS_EQUAL
    RenderPass Forward
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Core.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform mat4 skyboxView;
EndUniform()
TextureCube(0, 1, mainTex, mainSampler)

#if defined(VERTEX) && defined(MainPass)
    //layout(location = 0) in vec3 inPos;

    Out(0) vec3 texCoords;

    void main() {
        texCoords = pos;
        vec4 _pos = projection * skyboxView * vec4(pos, 1.0);
        gl_Position = _pos.xyww;
    }
#endif

#include Engine/ShaderLibrary/Core.glsl

#if defined(FRAGMENT) && defined(MainPass)
    In(0) vec3 texCoords;
    Out(0) vec4 fragColor;

    void main(){
        fragColor = texture(mainTex, texCoords);
    }
#endif