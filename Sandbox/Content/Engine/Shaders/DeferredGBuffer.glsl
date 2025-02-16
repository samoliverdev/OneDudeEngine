#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    MultiCompile _ SKINNED INSTANCING
    CullFace BACK
    DepthTest LESS
    lend Off
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

#undef SKINNED
#undef INSTANCING
#include Engine/ShaderLibrary/Vertex.glsl

#if defined(VERTEX) && defined(MainPass)
    out vec3 _pos;
    out vec3 _normal;
    out vec2 _texCoord;

    void main(){
        mat4 targetModelMatrix = GetModelMatrix();
        _pos = pos;
        _normal = normal;
        _texCoord = texCoord;
        gl_Position = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    uniform sampler2D mainTex;
    uniform vec4 color;

    in vec3 _pos;
    in vec3 _normal;
    in vec2 _texCoord;

    layout(location = 0) out vec3 gPosition;
    layout(location = 1) out vec3 gNormal;
    layout(location = 2) out vec4 gAlbedoSpec;

    void main(){
        vec4 texColor = texture(mainTex, _texCoord);
        if(texColor.a < 0.1) discard;

        gPosition = _pos;
        gNormal = normalize(_normal);
        gAlbedoSpec.rgb = texColor.rgb * color.rgb;
        gAlbedoSpec.a = 1.0;
    }
#endif