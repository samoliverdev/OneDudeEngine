#version 330 core

#pragma SupportInstancing false
#pragma CullFace NONE
#pragma DepthTest LESS
#pragma Blend Off

#ifdef VERTEX
#include Engine/ShaderLibrary/Vertex.glsl

uniform mat4 lightSpaceMatrix;
uniform sampler2D heightMap;
uniform float heightScale;
uniform vec2 heightmapTilling = vec2(1, 1);
uniform vec2 heightmapOffset = vec2(0, 0);

void main(){
    mat4 targetModelMatrix = GetModelMatrix();
    vec3 localPos = GetLocalPos().xyz;

    float height = texture(heightMap, texCoord * heightmapTilling + heightmapOffset).r; // uv + uvOffset
    localPos.y = height * heightScale;

    gl_Position = lightSpaceMatrix * targetModelMatrix * vec4(localPos, 1.0);
}
#endif

#ifdef FRAGMENT
void main(){}
#endif
