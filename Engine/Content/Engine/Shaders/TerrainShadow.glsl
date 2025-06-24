#pragma BeginPassDef
    Name MainPass
    SupportInstancing false
    CullFace NONE
    DepthTest LESS
    Blend Off
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(0, 0, Main)
    Uniform mat4 lightSpaceMatrix;
    Uniform float heightScale;
    Uniform vec2 heightmapTilling;// = vec2(1, 1);
    Uniform vec2 heightmapOffset;// = vec2(0, 0);
EndUniform()
Texture2D(0, 1, heightMap, heightMapSampler)

//#define USE_PERDRAW
uniform vec4 customData;

#if defined(VERTEX) && defined(MainPass)
    #include Engine/ShaderLibrary/Vertex.glsl

    /*uniform mat4 lightSpaceMatrix;
    uniform sampler2D heightMap;
    uniform float heightScale;
    uniform vec2 heightmapTilling = vec2(1, 1);
    uniform vec2 heightmapOffset = vec2(0, 0);*/

    void main(){
        #ifdef USE_PERDRAW
        vec2 _heightmapOffset = vec2(customData.x, customData.y);
        #else
        vec2 _heightmapOffset = heightmapOffset;
        #endif

        mat4 targetModelMatrix = GetModelMatrix();
        vec3 localPos = GetLocalPos().xyz;

        float height = texture(heightMap, texCoord * heightmapTilling + _heightmapOffset).r; // uv + uvOffset
        localPos.y = height * heightScale;

        gl_Position = lightSpaceMatrix * targetModelMatrix * vec4(localPos, 1.0);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    void main(){}
#endif
