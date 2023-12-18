#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main(){
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
#endif

#if defined(FRAGMENT)
void main(){            
    // gl_FragDepth = gl_FragCoord.z;
} 
#endif