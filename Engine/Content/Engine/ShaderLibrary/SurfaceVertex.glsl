#ifndef SURFACE_VERTEX
#define SURFACE_VERTEX

Out(0) vec2 outUV;
Out(1) vec3 outWorldPos;
Out(2) vec3 outWorldNormal;
Out(3) vec3 outT;
Out(4) vec3 outB;
Out(5) vec3 outN;
Out(6) vec4 perInstanceDataOut;

#ifndef CUSTOM_VERTEX

void SurfaceVertexDefault(){
    vec4 localPos = GetLocalPos();
    vec3 localNormal = GetLocalNormal();
    vec3 localTangent = GetLocalTangent();

    mat4 model = GetModelMatrix();

    vec3 T = normalize(vec3(model * vec4(localTangent, 0.0)));
    vec3 B = normalize(vec3(model * vec4(cross(localTangent, localNormal), 0.0)));
    vec3 N = normalize(vec3(model * vec4(localNormal, 0.0)));

    outUV = texCoord;

    outT = T;
    outB = B;
    outN = N;

    perInstanceDataOut = GetPerInstanceData();

    outWorldPos = vec3(model * localPos);
    outWorldNormal = mat3(transpose(inverse(model))) * localNormal;

    OutPosition = projection * view * model * localPos;
}

#else

// USER PROVIDES THIS:
void CustomVertex();

#endif

void main(){
    #ifdef CUSTOM_VERTEX
        CustomVertex();
    #else
        SurfaceVertexDefault();
    #endif
}

#endif