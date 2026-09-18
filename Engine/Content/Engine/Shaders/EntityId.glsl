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

#if defined(Vulkan_API)
#else
uniform int perDrawInt_0;
#endif

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
        #if defined(Vulkan_API)
        fragColor = -1;
        #else
        fragColor = perDrawInt_0; 
        #endif
    }
#endif