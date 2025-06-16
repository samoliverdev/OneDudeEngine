#pragma BeginPassDef
    Name MainPass
    CullFace NONE
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
    DepthMask False
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 a;
EndUniform()

BeginUniform(1, 0, PerDraw)
    Uniform mat4 model;
EndUniform()

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
EndUniform()

#if defined(VERTEX) && defined(MainPass)
    Attribute(0) vec3 vPosition;
    Attribute(1) vec3 vUv;
    //Attribute(2) vec3 _normal;

    Out(0) vec3 pos; 

    void main(){
        /*const vec3 positions[3] = vec3[3](
            vec3(-0.5, -0.5, 0.0),
            vec3(0.5, -0.5, 0.0),
            vec3(0.0, 0.5, 0.0)
        );
        //pos = positions[VertexIndex];
        OutPosition = vec4(positions[VertexIndex], 1.0);*/
        float dd = a.x;

        pos = vUv; //vPosition;
        OutPosition = projection * view * model * vec4(vPosition, 1.0);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    In(0) vec3 pos;

    Out(0) vec4 color;
    void main(){
        //color = vec4(0.0, 0.4, 1.0, 1.0);
        color = vec4(pos.xyz, 1.0);
    }
#endif