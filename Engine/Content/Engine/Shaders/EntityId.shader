Properties{
    Color4 color
    Vector4 sizeOffset 1 1 0 0
    Texture2D mainTex White
    Texture2D normalMap Normal
    Float normalStrength 1 0 10
    Texture2D emissionMap Black
    Color4 emissionColor 0 0 0 0
    Texture2D maskMap White
    Float occlusion 1 0 1
    Float metallic 0 0 1
    Float smoothness 0.5 0.0 1.0
    Float cutoff 0.5 0 1
}

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

Pass {
    #pragma Name MainPass
    #pragma SupportInstancing false
    #pragma DrawType _ SKINNED
    MultiCompile Opaque Blend

    MaterialData           { 
        int a;
}

    VertexInOut {
        Out(0) vec2 _texCoord;
    }

    FragInOut{ In(0) vec2 _texCoord;
        Out(0) int fragColor;}

    uniform int perDrawInt_0;

    void vertex ()  {
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }

    void fragment (               )
    { 
        fragColor = perDrawInt_0; 
    }
}