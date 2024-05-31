#version 330 core

#pragma MultiCompile _ SKINNED INSTANCING
#pragma CullFace BACK
#pragma DepthTest LESS
#pragma Blend Off

#if defined(VERTEX)
//layout (location = 0) in vec3 aPos;

//#define SKINNED

uniform mat4 lightSpaceMatrix;
//uniform mat4 model;

#include res/Engine/ShaderLibrary/Vertex.glsl

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