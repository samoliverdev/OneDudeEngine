#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;
layout (location = 2) in vec3 _normal;
layout (location = 5) in ivec4 _boneIds;
layout (location = 6) in vec4 _weights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 120;
const int MAX_BONE_INFLUENCE = 4;
//uniform mat4 finalBonesMatrices[MAX_BONES];

//uniform mat4 pose[MAX_BONES];
//uniform mat4 invBindPose[MAX_BONES];
uniform mat4 animated[MAX_BONES];

out vec3 pos;
out vec3 normal;
out vec2 texCoord;

flat out ivec4 boneIds;
out vec4 weights;

void main(){
    pos = _pos;
    normal = _normal;
    texCoord = _texCoord;
    boneIds = _boneIds;
    weights = _weights;

    /*
    vec4 totalPosition = vec4(0.0f);
    for(int i = 0; i < MAX_BONE_INFLUENCE; i++){
        if(_boneIds[i] == -1) continue;
        if(_boneIds[i] >= MAX_BONES){
            totalPosition = vec4(_pos,1.0f);
            break;
        }
        vec4 localPosition = animated[_boneIds[i]] * vec4(_pos,1.0f);
        totalPosition += localPosition * _weights[i];
        //vec3 localNormal = mat3(finalBonesMatrices[_boneIds[i]]) * _normal;
        //normal = localNormal;
    }

    mat4 viewModel = view * model;
    gl_Position =  projection * viewModel * totalPosition;
    */


    /*mat4 skin = (pose[boneIds.x] * invBindPose[boneIds.x]) * weights.x;
    skin += (pose[boneIds.y] * invBindPose[boneIds.y]) * weights.y;
    skin += (pose[boneIds.z] * invBindPose[boneIds.z]) * weights.z;
    skin += (pose[boneIds.w] * invBindPose[boneIds.w]) * weights.w;*/

    ///*
    mat4 skin = animated[boneIds.x] * weights.x +
    animated[boneIds.y] * weights.y +
    animated[boneIds.z] * weights.z +
    animated[boneIds.w] * weights.w;

    gl_Position = projection * view * model * skin * vec4(pos, 1.0);
    //*/

    //gl_Position = projection * view * model * vec4(pos, 1.0);
}
#endif

#if defined(FRAGMENT)
uniform sampler2D mainTex;
uniform vec4 color = vec4(1,1,1,1);

uniform int selectedBoneIndex = 36;

in vec3 pos;
in vec3 normal;
in vec2 texCoord;

flat in ivec4 boneIds;
in vec4 weights;

out vec4 fragColor;

float near = 0.1; 
float far  = 100.0; 
  
float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main() {
    vec4 outColor = texture(mainTex, texCoord) * color;
    if(outColor.a < 0.1) discard;

    fragColor = outColor;
    //fragColor = color;
    return;

    //outColor = vec4(normal, 1);
    //outColor = vec4(texCoord, 0, 1);

    //fragColor = outColor;
    //fragColor = weights;
    //return;

    //fragColor = vec4(boneIds.x, boneIds.x, boneIds.x,1);

    //fragColor = weights;

    for(int i = 0; i < 4; i++){
        if(boneIds[i] == selectedBoneIndex){
            if(weights[i] >= 0.7){
                fragColor = vec4(1,0,0,0) * weights[i];
            } else if(weights[i] >= 0.4 && weights[i] <= 0.6){
                fragColor = vec4(0,1,0,0) * weights[i];
            } else if(weights[i] >= 0.1){
                fragColor = vec4(1,1,0,0) * weights[i];
            }
            break;
        }
    }

    //float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
    //fragColor = vec4(vec3(depth), 1.0);
}
#endif