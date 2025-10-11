#pragma BeginPassDef
    Name MainPass
    DrawType _ SKINNED INSTANCING INSTANCINGMATRIX43
    CullFace BACK
    DepthTest LESS
    Blend Off
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(3, 0, ShadowData)
    Uniform mat4 lightSpaceMatrix;
EndUniform()

#if defined(VERTEX)
    //uniform mat4 lightSpaceMatrix;
    void main(){
        gl_Position = lightSpaceMatrix * GetModelMatrix() * GetLocalPos();
    }
#endif

#if defined(FRAGMENT)
    void main(){            
        // gl_FragDepth = gl_FragCoord.z;
    } 
#endif