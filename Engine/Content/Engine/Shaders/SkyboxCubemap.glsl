#pragma BeginPassDef
    Name MainPass
    CullFace NONE
    DepthTest LESS_EQUAL
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform mat4 skyboxView;
EndUniform()
TextureCube(0, 1, mainTex, mainSampler)

#if defined(VERTEX) && defined(MainPass)
    layout(location = 0) in vec3 inPos;

    out vec3 texCoords;

    //uniform mat4 skyboxView;
    //uniform mat4 projection;

    void main() {
        texCoords = inPos;
        
        //gl_Position = projection * view * vec4(inPos, 1.0);

        //mat4 rotView = mat4(mat3(view)); // remove translation from the view matrix
        //vec4 clipPos = projection * rotView * vec4(inPos, 1.0);
        //gl_Position = clipPos.xyww;

        vec4 pos = projection * skyboxView * vec4(inPos, 1.0);
        gl_Position = pos.xyww;
    }
#endif

#include Engine/ShaderLibrary/Core.glsl

#if defined(FRAGMENT) && defined(MainPass)
    in vec3 texCoords;
    out vec4 fragColor;

    //uniform samplerCube mainTex;

    void main(){
        fragColor = texture(mainTex, texCoords);
        //fragColor = vec4(1, 0, 0, 1);
        //fragColor = textureLod(mainTex, texCoords, 0);
    }
#endif