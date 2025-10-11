#pragma BeginProperties
    Color4 color
    Texture2D mainTex White
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    DrawType _ SKINNED INSTANCING INSTANCINGMATRIX43
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
EndUniform()

/*BeginUniform(1, 0, PerDraw)
    Uniform mat4 model;
EndUniform()

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
EndUniform()*/

#if defined(VERTEX) && defined(MainPass)
    /*Attribute(0) vec3 _pos;
    Attribute(1) vec2 _texCoord;
    Attribute(2) vec3 _normal;*/

    /*uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;*/

    /*Out(0) vec3 pos;
    Out(1) vec3 normal;
    Out(2) vec2 texCoord;*/

    Out(0) vec2 _texCoord;

    void main() {
        /*pos = _pos;
        normal = _normal;
        texCoord = _texCoord;*/

        //OutPosition = projection * view * model * vec4(pos, 1.0);

        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    //uniform sampler2D mainTex;
    //uniform vec4 color;

    /*In(0) vec3 pos;
    In(1) vec3 normal;*
    In(2) vec2 texCoord;*/

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