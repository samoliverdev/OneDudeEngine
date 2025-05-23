#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    MultiCompile _ SKINNED INSTANCING
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform int a;
EndUniform()

uniform int perDrawInt_0;

#if defined(VERTEX) && defined(MainPass)
    Out(0) vec2 _texCoord;

    void main(){
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    #include Engine/ShaderLibrary/Core.glsl

    In(0) vec2 _texCoord;
    Out(0) int fragColor;

    void main(){
        fragColor = perDrawInt_0; 
    }
#endif