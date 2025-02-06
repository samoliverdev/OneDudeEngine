#pragma BeginProperties
    Color4 color
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    MultiCompile _ SKINNED INSTANCING
#pragma EndPassDef

#include Engine/ShaderLibrary/Vertex.glsl

#if defined(VERTEX) && defined(MainPass)
    out vec2 _texCoord;

    void main(){
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord;
        gl_Position = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    #include Engine/ShaderLibrary/Core.glsl

    uniform sampler2D mainTex;
    uniform vec4 color;

    in vec2 _texCoord;
    out vec4 fragColor;

    void main(){
        vec4 texColor = textureSRGB(mainTex, _texCoord);
        if(texColor.a < 0.1) discard;
        fragColor = texColor * color;
    }
#endif