#version 330 core

#pragma MultiCompile _ SKINNED INSTANCING
#pragma CullFace BACK
#pragma DepthTest LESS
#pragma Blend Off

#if defined(VERTEX)
uniform mat4 lightSpaceMatrix;

#include Engine/ShaderLibrary/Vertex.glsl

void main(){
    //gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
    gl_Position = lightSpaceMatrix * GetModelMatrix() * GetLocalPos();
}
#endif

#if defined(FRAGMENT)
void main(){            
    // gl_FragDepth = gl_FragCoord.z;
} 
#endif