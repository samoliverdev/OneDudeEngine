Uniform vec3 _AmbientLight;
Uniform vec3 _IrradianceMapScale;
Uniform float _SkyLightIntensity;

#define MAX_DIRECTIONAL_LIGHT_COUNT 4
Uniform int _DirectionalLightCount;
Uniform vec4 _DirectionalLightColors[MAX_DIRECTIONAL_LIGHT_COUNT];
Uniform vec4 _DirectionalLightDirections[MAX_DIRECTIONAL_LIGHT_COUNT];
Uniform vec4 _DirectionalLightShadowData[MAX_DIRECTIONAL_LIGHT_COUNT];

#define MAX_OTHER_LIGHT_COUNT 16
Uniform int _OtherLightCount;
Uniform vec4 _OtherLightColors[MAX_OTHER_LIGHT_COUNT];
Uniform vec4 _OtherLightPositions[MAX_OTHER_LIGHT_COUNT];
Uniform vec4 _OtherLightDirections[MAX_OTHER_LIGHT_COUNT];
Uniform vec4 _OtherLightSpotAngles[MAX_OTHER_LIGHT_COUNT];
Uniform vec4 _OtherLightShadowData[MAX_OTHER_LIGHT_COUNT];

#define MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT 4
#define MAX_SHADOWED_OTHER_LIGHT_COUNT 16
#define MAX_CASCADE_COUNT 4
Uniform mat4 _DirectionalShadowMatrices[MAX_SHADOWED_DIRECTIONAL_LIGHT_COUNT * MAX_CASCADE_COUNT];
Uniform int _CascadeCount;
Uniform float _CascadeCullingSpheres[MAX_CASCADE_COUNT];
Uniform float _ShadowDistance;
Uniform vec4 _ShadowAtlasSize;
Uniform vec4 _ShadowDistanceFade;
Uniform mat4 _OtherShadowMatrices[MAX_SHADOWED_OTHER_LIGHT_COUNT];