#pragma BeginPassDef
    Name MainPass
    SupportInstancing false
    DrawType _ SKINNED
    RenderPass EntityId
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform int a;
EndUniform()

#if defined(VERTEX) && defined(MainPass)
    OutFlat(0) uint entityId;

    void main(){
        mat4 targetModelMatrix = GetModelMatrix();
        entityId = floatBitsToUint(GetPerInstanceData().x);
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    #include Engine/ShaderLibrary/Core.glsl

    InFlat(0) uint entityId;
    Out(0) int fragColor;

    void main(){
        fragColor = int(entityId);
    }
#endif
