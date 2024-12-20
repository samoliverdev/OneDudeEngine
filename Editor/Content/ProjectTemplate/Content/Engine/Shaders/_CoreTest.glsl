#version 330 core
#pragma SupportInstancing true
#pragma MultiCompile _ SKINNED INSTANCING

#include Engine/Shaders/_CoreShader.glsl

uniform sampler2D mainTex;
uniform vec4 color = vec4(1,1,1,1);

vec4 VertexFunc(mat4 projection, mat4 view, mat4 model, vec4 pos){
    return projection * view * model * pos;
}

vec4 FragmentFunc(){
    vec4 texColor = texture(mainTex, _texCoord);
    return texColor * color;
}