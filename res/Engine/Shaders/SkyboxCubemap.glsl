#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 inPos;

out vec3 texCoords;

uniform mat4 view;
uniform mat4 projection;

void main() {
    texCoords = inPos;
    gl_Position = projection * view * vec4(inPos, 1.0);

    //mat4 rotView = mat4(mat3(view)); // remove translation from the view matrix
    //vec4 clipPos = projection * rotView * vec4(inPos, 1.0);
    //gl_Position = clipPos.xyww;
}
#endif

#if defined(FRAGMENT)
in vec3 texCoords;
out vec4 fragColor;

uniform samplerCube mainTex;

#include res/Engine/ShaderLibrary/Core.glsl

void main() {
    fragColor = texture(mainTex, texCoords);
    //fragColor = textureLod(mainTex, texCoords, 0);
}
#endif