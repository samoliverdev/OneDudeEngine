#define MAX_DIRECTIONAL_LIGHT_COUNT 4
#define MAX_OTHER_LIGHT_COUNT 16
#define MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT 4
#define MAX_SHADOWED_OTHER_LIGHT_COUNT 16
#define MAX_CASCADE_COUNT 4

BeginUniform(2, 0, PipelineData)
    Uniform mat4 _DirectionalShadowMatrices[MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT * MAX_CASCADE_COUNT];
    Uniform mat4 _OtherShadowMatrices[MAX_SHADOWED_OTHER_LIGHT_COUNT];

    Uniform vec4 _DirectionalLightColors[MAX_DIRECTIONAL_LIGHT_COUNT];
    Uniform vec4 _DirectionalLightDirections[MAX_DIRECTIONAL_LIGHT_COUNT];
    Uniform vec4 _DirectionalLightShadowData[MAX_DIRECTIONAL_LIGHT_COUNT];
    Uniform vec4 _OtherLightColors[MAX_OTHER_LIGHT_COUNT];
    Uniform vec4 _OtherLightPositions[MAX_OTHER_LIGHT_COUNT];
    Uniform vec4 _OtherLightDirections[MAX_OTHER_LIGHT_COUNT];
    Uniform vec4 _OtherLightSpotAngles[MAX_OTHER_LIGHT_COUNT];
    Uniform vec4 _OtherLightShadowData[MAX_OTHER_LIGHT_COUNT];
    Uniform vec4 _CascadeCullingSpheres[MAX_CASCADE_COUNT];

    Uniform vec4 _ShadowAtlasSize;
    Uniform vec4 _ShadowDistanceFade;
    Uniform vec4 _AmbientLight;
    Uniform vec4 _IrradianceMapScale;

    Uniform float _SkyLightIntensity;
    Uniform float _ShadowDistance;
    Uniform float _Pad0;
    Uniform float _Pad1;
    
    Uniform int _DirectionalLightCount;
    Uniform int _OtherLightCount;
    Uniform int _CascadeCount;
    Uniform int _Pad2;
EndUniform()