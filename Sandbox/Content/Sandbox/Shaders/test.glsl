#pragma BeginPassDef
    Name MainPass
    CullFace NONE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

#if defined(VERTEX) && defined(MainPass)
    Out(0) vec2 outPos; 

    void main(){
        mat4 targetModelMatrix = GetModelMatrix();
        outPos = texCoord;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    In(0) vec2 outPos;
    Out(0) vec4 fragColor;

    void main(){
        //fragColor = vec4(0.0, 0.4, 1.0, 1.0);
        fragColor = vec4(outPos.xy, 0, 1.0);
    }
#endif