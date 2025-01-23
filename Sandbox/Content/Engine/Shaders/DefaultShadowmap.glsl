#pragma BeginPassDef
    Name Shadowmap
    CullFace BACK
    DepthTest LESS
    Blend Off
    MultiCompile _ SKINNED INSTANCING
#pragma EndPassDef

#if defined(Shadowmap) && defined(VERTEX)
uniform mat4 lightSpaceMatrix;

#include Engine/ShaderLibrary/Vertex.glsl

void main(){
    gl_Position = lightSpaceMatrix * GetModelMatrix() * GetLocalPos();
}
#endif

#if defined(Shadowmap) && defined(FRAGMENT)
void main(){            
    // gl_FragDepth = gl_FragCoord.z;
} 
#endif