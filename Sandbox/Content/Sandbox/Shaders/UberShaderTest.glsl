#version 330 core

#pragma MultiCompile _ Fade Instancing
#pragma MultiCompile _ COLOR1

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;
layout (location = 2) in vec3 _normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 pos; 
out vec3 normal;

void main() {
    pos = _pos;
    normal = _normal;
    gl_Position = projection * view * model * vec4(pos, 1.0);
}
#endif

#if defined(FRAGMENT)
in vec3 pos;
in vec3 normal;
out vec4 color;

void main(){
    #ifdef COLOR1
    color = vec4(vec3(1,0,0), 1) * dot(vec3(0, 1, 0), normalize(normal));
    #else
    color = vec4(vec3(0,0,1), 1) * dot(vec3(0, 1, 0), normalize(normal));
    #endif
}
#endif