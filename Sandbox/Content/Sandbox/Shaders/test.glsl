#pragma BeginPassDef
    Name MainPass
    CullFace NONE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

/*BeginAttribute()
    Attribute(0) vec3 vPosition;
EndAttribute()*/

#if defined(VERTEX) && defined(MainPass)
    //Attribute(0) vec3 vPosition;

    //Out(0) vec3 pos; 

    void main(){
        const vec3 positions[3] = vec3[3](
            vec3(-0.5, -0.5, 0.0),
            vec3(0.5, -0.5, 0.0),
            vec3(0.0, 0.5, 0.0)
        );
        //pos = positions[gl_VertexIndex];
        OutPosition = vec4(positions[gl_VertexIndex], 1.0);

        //pos = vPosition;
        //OutPosition = vec4(pos, 1.0);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    //In(0) vec3 pos;

    Out(0) vec4 color;
    void main(){
        color = vec4(0.0, 0.4, 1.0, 1.0);
        //color = vec4(pos.xyz, 1.0);
    }
#endif