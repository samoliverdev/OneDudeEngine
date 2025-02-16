#pragma BeginPassDef
    Name MainPass
    SupportInstancing false
    CullFace NONE
    DepthTest LESS
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
    DepthMask False
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

#if defined(VERTEX) && defined(MainPass)
    layout (location = 0) in vec3 _pos;
    layout (location = 1) in vec2 _texCoord;
    layout (location = 2) in vec3 _normal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec3 pos;
    out vec3 normal;
    out vec2 texCoord;

    void main(){
        pos = _pos;
        normal = _normal;
        texCoord = _texCoord;

        gl_Position = projection * view * model * vec4(pos, 1.0);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    uniform sampler2D mainTex;
    uniform vec4 color = vec4(1.0);

    in vec3 pos;
    in vec3 normal;
    in vec2 texCoord;

    out vec4 fragColor;

    void main(){
        vec4 texColor = texture(mainTex, texCoord);
        fragColor = texColor * color;
    }
#endif