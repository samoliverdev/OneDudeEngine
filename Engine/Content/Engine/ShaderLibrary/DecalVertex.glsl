#ifndef DECAL_VERTEX_GLSL
#define DECAL_VERTEX_GLSL

flat out vec4 vDecalInvRow0;
flat out vec4 vDecalInvRow1;
flat out vec4 vDecalInvRow2;
flat out vec4 vDecalInvRow3;

out vec3 decalNormalWS;
out vec3 objWorldPos;
flat out vec4 perInstanceData;

void main() {
    mat4 model = GetModelMatrix();

    OutPosition = projection * view * model * GetLocalPos();
    objWorldPos = (model * vec4(0, 0, 0, 1)).xyz;
    perInstanceData = GetPerInstanceData();

    mat4 invModel = inverse(model);

    vDecalInvRow0 = invModel[0];
    vDecalInvRow1 = invModel[1];
    vDecalInvRow2 = invModel[2];
    vDecalInvRow3 = invModel[3];

    decalNormalWS = normalize((model * vec4(0.0, 0.0, 1.0, 0.0)).xyz);
}

#endif