#pragma BeginPassDef
    Name MainPass
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Common.glsl

Texture2D(0, 5,  mainTex,        mainTexSampler)
Texture2D(0, 7,  gNormal,        gNormalSampler)
Texture2D(0, 7,  gAlbedoSpec,    gAlbedoSpecSampler)
Texture2D(0, 12, gDepth,         gDepthSampler)
Texture2D(0, 11, noise,          noiseSampler)
Texture2D(0, 12, lastIndirect, lastIndirectSampler)

BeginUniform(0, 0, Main)
    Uniform vec2  screenSize;
    Uniform float sampleCount;
    Uniform float sampleRadius;
    Uniform float sliceCount;
    Uniform float hitThickness;
    Uniform float giIntensity;
    Uniform float aoIntensity;
    Uniform float useScreenSpaceSampling; // 0 or 1
    Uniform float backfaceLighting;

    Uniform float cameraNear;
    Uniform float cameraFar;
    Uniform float halfProjScale;
EndUniform()



// ============================================================
// VERTEX
// ============================================================
#if defined(VERTEX) && defined(MainPass)
    layout (location = 0) in vec3 _pos;
    layout (location = 1) in vec2 _texCoord;

    out vec2 vUV;

    void main(){
        vUV = _texCoord;
        gl_Position = vec4(_pos, 1.0);
    }
#endif

// ============================================================
// FRAGMENT
// ============================================================
#if defined(FRAGMENT) && defined(MainPass)

in vec2 vUV;
out vec4 FragColor;

#include Engine/ShaderLibrary/Vertex.glsl

#define stepCount sampleCount
#define thickness hitThickness
#define radius sampleRadius

const float useLinearThickness = 0; //false;

const float temporalDirection = 0;
const float temporalOffset = 0;
const float expFactor = 2;

const uint MAX_RAY = 32u;

uint globalOccludedBitfield = 0u;

float luminance(vec3 c){
    return dot(c, vec3(0.2126,0.7152,0.0722));
}

float sampleDepth(vec2 uv){
    return texture(gDepth, uv).r;
}

vec3 sampleNormal(vec2 uv){
    vec3 n = unpack_normal_octahedron(texture(gNormal, uv).rg);
    return normalize(mat3(view) * n);

    //return normalize(texture(gNormal, uv).xyz);
}

vec3 sampleBeauty(vec2 uv){
    return texture(mainTex, uv).rgb;
}

vec3 getViewPosition(vec2 uv, float depth){
    vec3 wp = reconstructWorldPos(
        uv,
        texture(gDepth, uv).r,
        invProjection,
        invView
    );
    return (view * vec4(wp, 1.0)).xyz;

    /*vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 view = invProjection * clip;
    return view.xyz / view.w;*/
}

vec2 GTAOFastAcos(vec2 value){
    vec2 outVal = abs(value) * -0.156583 + 1.57079632679;
    outVal *= sqrt(1.0 - abs(value));

    vec2 r;
    r.x = value.x >= 0.0 ? outVal.x : 3.14159265 - outVal.x;
    r.y = value.y >= 0.0 ? outVal.y : 3.14159265 - outVal.y;

    return r;
}

float spatialOffsets(vec2 position){
    return 0.25 * float((int(position.y) - int(position.x)) & 3);
}

uint countBits(uint x){
    //return bitCount(x);
    return uint(bitCount(x));
}

vec3 horizonSampling(
    float directionIsRight,
    float RADIUS,
    vec3 viewPosition,
    vec2 slideDirTexelSize,
    float initialRayStep,
    vec2 uvNode,
    vec3 viewDir,
    vec3 viewNormal,
    float n
){
    float stepRadius;

    if(useScreenSpaceSampling > 0)
        stepRadius = RADIUS * (screenSize.x * 0.5) / 16.0;
    else
        stepRadius = max(RADIUS * halfProjScale / (-viewPosition.z), float(stepCount));

    stepRadius /= (float(stepCount) + 1.0);

    float radiusVS = max(1.0, float(stepCount - 1u)) * stepRadius;

    vec2 uvDirection = directionIsRight > 0 ? vec2(1,-1) : vec2(-1,1);
    float samplingDirection = directionIsRight > 0 ? 1.0 : -1.0;

    vec3 color = vec3(0);

    vec3 lastSampleViewPosition = viewPosition;

    for(uint i=0u;i<stepCount;i++){
        float offset = pow(abs(stepRadius * (float(i) + initialRayStep) / radiusVS), expFactor) * radiusVS;
        vec2 uvOffset = slideDirTexelSize * max(offset, float(i)+1.0);

        vec2 sampleUV = uvNode + uvOffset*uvDirection;

        if(sampleUV.x <= 0 ||sampleUV.y <= 0 ||sampleUV.x >= 1 || sampleUV.y >= 1) break;

        float depth = sampleDepth(sampleUV);

        vec3 sampleViewPosition = getViewPosition(sampleUV, depth);

        vec3 pixelToSample = normalize(sampleViewPosition-viewPosition);

        float linearThicknessMultiplier = useLinearThickness > 0 ? clamp((-sampleViewPosition.z) / cameraFar, 0.0, 1.0) * 100.0 : 1.0;

        vec3 pixelToSampleBackface = normalize(sampleViewPosition - linearThicknessMultiplier*viewDir*thickness - viewPosition);

        vec2 frontBackHorizon = vec2(dot(pixelToSample,viewDir), dot(pixelToSampleBackface,viewDir));

        frontBackHorizon = GTAOFastAcos(clamp(frontBackHorizon,-1,1));

        frontBackHorizon = clamp((samplingDirection*-frontBackHorizon - (n-1.57079632679))/3.14159265, 0.0,1.0);

        if(directionIsRight > 0) frontBackHorizon = frontBackHorizon.yx;

        float minHorizon = frontBackHorizon.x;
        float maxHorizon = frontBackHorizon.y;

        uint startHorizonInt = uint(frontBackHorizon.x * float(MAX_RAY));
        uint angleHorizonInt = uint(ceil((maxHorizon-minHorizon)*float(MAX_RAY)));

        uint angleHorizonBitfield = angleHorizonInt>0u ? (0xFFFFFFFFu >> (32u - MAX_RAY + (MAX_RAY-angleHorizonInt))) : 0u;

        uint currentOccludedBitfield = angleHorizonBitfield << startHorizonInt;

        currentOccludedBitfield &= ~globalOccludedBitfield;

        globalOccludedBitfield |= currentOccludedBitfield;

        uint numOccludedZones = countBits(currentOccludedBitfield);

        if(numOccludedZones > 0u){
            vec3 lightColor = sampleBeauty(sampleUV);

            if(luminance(lightColor)>0.001){
                vec3 lightDirectionVS = normalize(pixelToSample);

                float normalDotLightDirection = clamp(dot(viewNormal,lightDirectionVS),0,1);

                if(normalDotLightDirection > 0.001){

                    vec3 lightNormalVS = sampleNormal(sampleUV);

                    float lightNormalDotLightDirection = dot(lightNormalVS,-lightDirectionVS);

                    float d = sign(lightNormalDotLightDirection) < 0 ? abs(lightNormalDotLightDirection) * backfaceLighting : abs(lightNormalDotLightDirection);

                    if(backfaceLighting > 0 && dot(lightNormalVS, viewDir) > 0)
                        lightNormalDotLightDirection = d;
                    else
                        lightNormalDotLightDirection = clamp(lightNormalDotLightDirection, 0, 1);

                    color += float(numOccludedZones) / float(MAX_RAY) * lightColor * normalDotLightDirection * lightNormalDotLightDirection;
                }
            }
        }

        lastSampleViewPosition = sampleViewPosition;
    }

    return color;
}

void main(){
    float depth = sampleDepth(vUV);

    if(depth >= 1.0) discard;

    vec3 viewPosition = getViewPosition(vUV,depth);

    vec3 viewNormal = sampleNormal(vUV);

    vec3 viewDir = normalize(-viewPosition);

    vec2 screenCoord = gl_FragCoord.xy;

    float noiseOffset = spatialOffsets(screenCoord);

    float noiseDirection = fract(sin(dot(screenCoord,vec2(12.9898,78.233)))*43758.5453);

    float noiseJitterIdx = temporalDirection*0.02;

    float initialRayStep =
    fract(noiseOffset + temporalOffset) +
    fract(sin(dot(vUV+noiseJitterIdx,vec2(12.9898,78.233)))*43758.5453);

    float ao = 0.0;
    vec3 color = vec3(0);

    for(uint i=0u;i<sliceCount;i++){
        float rotationAngle = (float(i)+noiseDirection+temporalDirection) * 3.14159265/float(sliceCount);

        vec3 sliceDir = vec3(cos(rotationAngle),sin(rotationAngle),0);

        vec2 slideDirTexelSize = sliceDir.xy/screenSize;

        vec3 planeNormal = normalize(cross(sliceDir,viewDir));

        vec3 tangent = cross(viewDir,planeNormal);

        vec3 projectedNormal = viewNormal - planeNormal*dot(viewNormal,planeNormal);

        vec3 projectedNormalNormalized = normalize(projectedNormal);

        float cos_n = clamp(dot(projectedNormalNormalized,viewDir),-1,1);

        float n = -sign(dot(projectedNormal,tangent)) * acos(cos_n);

        globalOccludedBitfield = 0u;

        color += horizonSampling(1, radius, viewPosition, slideDirTexelSize, initialRayStep, vUV, viewDir, viewNormal, n);

        color += horizonSampling(0, radius, viewPosition, slideDirTexelSize, initialRayStep, vUV, viewDir, viewNormal, n);

        ao += float(bitCount(globalOccludedBitfield))/float(MAX_RAY);
    }

    ao /= float(sliceCount);

    ao = clamp(pow(1.0-clamp(ao, 0, 1), aoIntensity), 0, 1);

    color /= float(sliceCount);

    color *= giIntensity;

    float maxLuminance = 7.0;

    float currentLuminance = luminance(color);

    float scale = currentLuminance > maxLuminance ? maxLuminance / currentLuminance : 1.0;

    color *= scale;

    FragColor = vec4(color,ao);
}

#endif
