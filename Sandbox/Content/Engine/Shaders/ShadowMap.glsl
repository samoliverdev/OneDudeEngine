#pragma BeginPassDef
    Name MainPass
    MultiCompile _ SKINNED INSTANCING
    CullFace BACK
    DepthTest LESS
    Blend Off
#pragma EndPassDef

#include Engine/ShaderLibrary/Vertex.glsl

#if defined(VERTEX) && defined(MainPass)
    uniform mat4 lightSpaceMatrix;
    void main(){
        gl_Position = lightSpaceMatrix * GetModelMatrix() * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    void main(){            
        // gl_FragDepth = gl_FragCoord.z;
    } 
#endif