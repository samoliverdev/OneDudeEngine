#version 330 core

#include _BaseShaderInputs.glsl

uniform sampler2D mainTex;
uniform vec4 color = vec4(1,1,1,1);

vec4 _vertex(ShaderData data){
    return projection * view * model * vec4(data.pos, 1.0);
}

vec4 _fragment(ShaderData data){
    return texture(mainTex, data.texCoord) * color;
}

#define VertexFunction _vertex
#define FragmentFunction _fragment
#include _BaseShader.glsl