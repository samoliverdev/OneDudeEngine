#pragma BeginPassDef
    Name MainPass
    DrawType _ SKINNED INSTANCING INSTANCINGMATRIX43 SKINNED2
    RenderPass DirectionalShadow OtherShadow
    CullFace BACK
    DepthTest LESS
    Blend Off
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

#if defined(VERTEX)
    //uniform mat4 lightSpaceMatrix;
    void main(){
        OutPosition = projection * view * GetModelMatrix() * GetLocalPos();
    }
#endif

#if defined(FRAGMENT)
    void main(){            
        // gl_FragDepth = gl_FragCoord.z;
    } 
#endif