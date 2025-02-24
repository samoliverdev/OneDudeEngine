#pragma BeginProperties
    Color4 color
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    MultiCompile _ SKINNED INSTANCING
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

#if defined(VERTEX) && defined(MainPass)
    Out(0) vec2 _texCoord;

    void main(){
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    #include Engine/ShaderLibrary/Core.glsl

    //uniform sampler2D mainTex;
    //uniform vec4 color;

    In(0) vec2 _texCoord;
    Out(0) vec4 fragColor;

    void main(){
        vec4 texColor = vec4(0, 1, 0, 1); //textureSRGB(mainTex, _texCoord);
        if(texColor.a < 0.1) discard;
        fragColor = texColor;// * color;
    }
#endif