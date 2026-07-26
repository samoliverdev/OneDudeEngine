#pragma BeginPassDef
    Name MainPass
    CullFace NONE
    DepthTest LESS_EQUAL
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Core.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform mat4 skyboxView;
EndUniform()
TextureCube(0, 1, mainTex, mainSampler)

#if defined(VERTEX) && defined(MainPass)
    layout(location = 0) in vec3 inPos;

    out vec3 texCoords;

    void main() {
        texCoords = inPos;
        vec4 pos = projection * skyboxView * vec4(inPos, 1.0);
        gl_Position = pos.xyww;
    }
#endif

#include Engine/ShaderLibrary/Core.glsl

#if defined(FRAGMENT) && defined(MainPass)
    in vec3 texCoords;
    out vec4 fragColor;

    void main(){
        fragColor = texture(mainTex, texCoords);
    }
#endif