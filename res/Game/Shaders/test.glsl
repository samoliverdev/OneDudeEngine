#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 position;

out vec3 _pos; 

void main() {
    _pos = position;
    gl_Position = vec4(_pos, 1);
}
#endif

#if defined(FRAGMENT)
in vec3 _pos;
out vec4 color;

void main(){
    color = vec4(_pos.xyz, 1);
}
#endif