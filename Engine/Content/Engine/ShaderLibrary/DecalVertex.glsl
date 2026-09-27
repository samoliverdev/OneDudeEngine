#ifndef DECAL_VERTEX_GLSL
#define DECAL_VERTEX_GLSL

OutFlat(0) vec4 vDecalInvRow0;
OutFlat(1) vec4 vDecalInvRow1;
OutFlat(2) vec4 vDecalInvRow2;
OutFlat(3) vec4 vDecalInvRow3;

Out(4) vec3 decalNormalWS;
Out(5) vec3 objWorldPos;
OutFlat(6) vec4 perInstanceData;

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