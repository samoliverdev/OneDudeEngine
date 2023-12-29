#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 inPos;

out vec3 texCoords;

uniform mat4 view;
uniform mat4 projection;

void main() {
    texCoords = inPos;
    gl_Position = projection * view * vec4(inPos, 1.0);
}
#endif

#if defined(FRAGMENT)
uniform samplerCube mainTex;

in vec3 texCoords;

out vec4 fragColor;

void main() {
    fragColor = texture(mainTex, texCoords);
}
#endif