#pragma BeginProperties
    Color4 color
    Texture2D mainTex White
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    DrawType _ SKINNED INSTANCING INSTANCINGMATRIX43
    RenderPass Forward Deferred DeferredCopy
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
EndUniform()

#if defined(VERTEX) && defined(MainPass)
    Out(0) vec2 _texCoord;

    void main() {
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    In(0) vec2 _texCoord;
    Out(0) vec4 fragColor;

    const float near = 0.1; 
    const float far  = 100.0; 
    
    float LinearizeDepth(float depth) {
        float z = depth * 2.0 - 1.0; // back to NDC 
        return (2.0 * near * far) / (far + near - z * (far - near));	
    }

    void main() {
        vec4 outColor = /*texture(mainTex, texCoord) **/ color;
        if(outColor.a < 0.1) discard;

        fragColor = outColor;
    }
#endif