#pragma BeginProperties
    Texture2D mainTex White
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
#pragma EndPassDef

#if defined(VERTEX) && defined(MainPass)
    layout(location = 0) in vec3 vPos;
    layout(location = 1) in vec2 vTexCoord;

    out vec3 pos;
    out vec2 texCoord;

    void main() {
        pos = vPos;
        texCoord = vTexCoord;
        gl_Position = vec4(pos, 1.0);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    uniform sampler2D mainTex;

    in vec3 pos;
    in vec2 texCoord;

    out vec4 fragColor;

    void main() {
        //fragColor = vec4(1, 0, 0, 1);
        fragColor = texture(mainTex, texCoord);
    }
#endif