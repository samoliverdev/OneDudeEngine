#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(position, 1);
}
#endif

#if defined(FRAGMENT)
uniform vec3 color;

out vec4 outColor;

void main(){
    outColor = vec4(color.xyz, 1);
}
#endif