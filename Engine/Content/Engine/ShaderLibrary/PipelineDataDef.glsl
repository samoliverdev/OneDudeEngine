//INFO: Need sync with RenderContext.h
#define MAX_DIRECTIONAL_LIGHT_COUNT 4
#define MAX_OTHER_LIGHT_COUNT 16
#define MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT 1
#define MAX_SHADOWED_OTHER_LIGHT_COUNT 16
#define MAX_CASCADE_COUNT 4

BeginUniform(0, 1, PipelineData)
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

TextureCube(0, 2, _IrradianceMap, _IrradianceMapSampler)
TextureCube(0, 3, _PrefilterMap, _PrefilterMapSampler)
Texture2D(0, 4, _BrdfLUT, _BrdfLUTSampler)
Texture2DArray(0, 5, _DirectionalShadowAtlas, _DirectionalShadowAtlasSampler)
Texture2DArray(0, 6, _OtherShadowAtlas, _OtherShadowAtlasSampler)
//Uniform mediump sampler2DArray _DirectionalShadowAtlas;
//Uniform mediump sampler2DArray _OtherShadowAtlas;

TextureCube(0, 7, _EnvironmentMap, _EnvironmentMapSampler)