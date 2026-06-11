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

    Uniform float _HalfProjScale;
    Uniform vec4 _Resolution;
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

const float PI      = 3.14159265359;
const float HALF_PI = 1.57079632679;
const float TWO_PI  = 6.28318530718;

//const vec4 _Resolution = vec4(800,600, 0, 0); // xy = size, zw = 1/size

const float _Radius = 12;
const float _GIIntensity = 10;
const float _AOIntensity = 1;
const float _Thickness = 1;
const float _ExpFactor = 2;
const float _BackfaceLighting = 0;
const int _StepCount = 8;
const int _SliceCount = 2;

const float _TemporalDirection = 1;
const float _TemporalOffset = 1;

//const float _HalfProjScale = 0;

//const float _CameraFar = 100;
const float _UseLinearThickness = 0;      // 0 or 1
const float _UseScreenSpaceSampling = 1;  // 0 or 1

#define saturate(x) clamp(x, 0.0, 1.0)

float SampleDepth(vec2 uv){
    return texture(gDepth, uv).r;
} 

vec3 GetViewPos(vec2 uv, float depth){
    vec3 wp = reconstructWorldPos(
        uv,
        depth,
        invProjection,
        invView
    );
    return (view * vec4(wp, 1.0)).xyz;
}

vec3 SampleNormal(vec2 uv){
    vec3 n = unpack_normal_octahedron(texture(gNormal, uv).rg);
    return normalize(mat3(view) * n);
}

vec3 SampleColor(vec2 uv){
    return texture(mainTex, uv).rgb;
}

uint CountBits(uint v){
    /*v = v - ((v >> 1u) & 0x55555555u);
    v = (v & 0x33333333u) + ((v >> 2u) & 0x33333333u);
    return ((v + (v >> 4u) & 0xF0F0F0Fu) * 0x1010101u) >> 24u;*/

    return uint(bitCount(v));
}

vec2 GTAOFastAcos(vec2 x){
    vec2 res = abs(x) * -0.156583 + HALF_PI;
    res *= sqrt(1 - abs(x));

    return vec2(
        x.x >= 0 ? res.x : PI - res.x,
        x.y >= 0 ? res.y : PI - res.y
    );
}

float SpatialOffset(vec2 pixel){
    int x = int(pixel.x);
    int y = int(pixel.y);
    return 0.25 * float((y - x) & 3);
}

#define Test

vec3 HorizonSampling(
    float directionRight,
    float radius,
    vec3 viewPos,
    vec2 dirTexel,
    float initialStep,
    vec2 uv,
    vec3 viewDir,
    vec3 normal,
    float n,
    inout uint globalMask
){
    vec3 color = vec3(0.0, 0.0, 0.0);

    #ifndef Test
    float stepRadius = max(radius * _HalfProjScale / -viewPos.z, float(_StepCount));
    #else
    float stepRadius;

    if(_UseScreenSpaceSampling > 0.5){
        // matches three.js
        stepRadius = radius * (_Resolution.x * 0.5) / 16.0;
    }else{
        stepRadius = max(radius * _HalfProjScale / -viewPos.z, float(_StepCount));
    }
    #endif

    stepRadius /= (_StepCount + 1);

    float radiusVS = max(1, _StepCount - 1) * stepRadius;

    vec2 uvDir = directionRight > 0.5 ? vec2(1, 1) : vec2(-1, -1);
    float signDir = directionRight > 0.5 ? 1 : -1;

    //[loop]
    for(int i = 0; i < _StepCount; i++){
        float offset = pow(abs(stepRadius * (i + initialStep) / radiusVS), _ExpFactor) * radiusVS;
        vec2 uvOffset = dirTexel * max(offset, i + 1);

        vec2 suv = uv + uvOffset * uvDir;

        if(suv.x <= 0 || suv.y <= 0 || suv.x >= 1 || suv.y >= 1) break;

        float d = SampleDepth(suv);
        vec3 samplePos = GetViewPos(suv, d);

        vec3 dir = normalize(samplePos - viewPos);

        #ifndef Test
        vec3 backDir = normalize(samplePos - viewDir * _Thickness - viewPos);
        #else
        float linearThickness = 1.0;
        if(_UseLinearThickness > 0.5){
            // viewPos.z is NEGATIVE in view space
            float depthFactor = saturate(-samplePos.z / cameraFar); //_CameraFar);
            linearThickness = depthFactor * 100.0;
        }

        vec3 backDir = normalize(samplePos - viewDir * (_Thickness * linearThickness) - viewPos);
        #endif

        vec2 horizons = vec2(dot(dir, viewDir), dot(backDir, viewDir));
        //float2 horizons = float2(dot(dir, normal), dot(backDir, normal));
        horizons = GTAOFastAcos(clamp(horizons, -1, 1));

        horizons = (signDir * -horizons - (n - HALF_PI)) / PI;
        horizons = clamp(horizons, 0, 1);

        if(directionRight > 0.5) horizons = horizons.yx;

        float minH = horizons.x;
        float maxH = horizons.y;

        //uint start = (uint)(minH * 32);
        //uint len = (uint)ceil((maxH - minH) * 32);
        uint start = uint(minH * 32.0);
        uint len   = uint(ceil((maxH - minH) * 32.0));

        //uint mask = len > 0 ? (0xFFFFFFFF >> (32 - len)) : 0;
        uint mask = len > 0u ? (0xFFFFFFFFu >> (32u - len)) : 0u;
        
        mask <<= start;

        mask &= ~globalMask;

        globalMask |= mask;

        uint hits = CountBits(mask);

        if(hits > 0){
            vec3 light = SampleColor(suv);

            if(dot(light, vec3(1.0)) > 0.001){
                float ndl = saturate(dot(normal, dir));

                if(ndl > 0.001){
                    vec3 lightNormal = SampleNormal(suv);
                    float lndl = dot(lightNormal, -dir);

                    float d = (sign(lndl) < 0) ? abs(lndl) * _BackfaceLighting : abs(lndl);

                    lndl = (_BackfaceLighting > 0 && dot(lightNormal, viewDir) > 0) ? d : saturate(lndl);

                    color += (float(hits) / 32.0) * light * ndl * lndl;
                }
            }
        }
    }

    return color;
}

float InterleavedGradientNoise(vec2 pixel){
    // pixel = integer pixel coords (VERY important)
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(pixel, magic.xy)));
}

float Rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453123);
}

// https://blog.demofox.org/2022/01/01/interleaved-gradient-noise-a-different-kind-of-low-discrepancy-sequence/
float randf(int x, int y) {
    return mod(52.9829189 * mod(0.06711056 * float(x) + 0.00583715 * float(y), 1.0), 1.0);
}

float randf(vec2 st) {
    return mod(52.9829189 * mod(0.06711056 * float(st.x) + 0.00583715 * float(st.y), 1.0), 1.0);
}

float blueNoise(vec2 uv){
    return randf(int(gl_FragCoord.x), int(gl_FragCoord.y)) - 0.5;
    //return texture(noise, uv * screenSize / 64.0).r - 0.5;
}

//======================================================
// MAIN GI
//======================================================
vec4 ComputeSSGI(vec2 uv){
    float depth = SampleDepth(uv);
    //if(depth >= 1.0) discard;

    vec3 viewPos = GetViewPos(uv, depth);
    vec3 normal = SampleNormal(uv);
    vec3 viewDir = normalize(-viewPos);

    //float noise = fract(sin(dot(uv * _Resolution.xy, vec2(12.9898,78.233))) * 43758.5453);
    //float initialStep = fract(noise + _TemporalOffset);

    //vec2 pixel = uv * _Resolution.xy;// _ScreenParams.xy;
    vec2 pixel = floor(uv * _Resolution.xy);
    //float2 pixel = floor(uv * _ScreenParams.xy);

    //float noiseOffset = fract(0.25 * mod(pixel.y - pixel.x, 4.0)); // spatial offset (GTAO style)
    float noiseOffset = SpatialOffset(pixel);
    float noiseDirection = InterleavedGradientNoise(pixel); // interleaved gradient noise
    float temporalOffset = _TemporalOffset;// 1.0; // temporal (if disabled, set to 1)
    float temporalDirection = _TemporalDirection;// 1.0;
    float noiseJitterIdx = temporalDirection * 0.02; // jitter index (optional but in your code)
    float initialStep = fract(noiseOffset + temporalOffset) + Rand((uv + noiseJitterIdx) * 2.0 - 1.0); // initial ray step (THIS IS VERY IMPORTANT)

    vec3 color = vec3(0);
    float ao = 0;

    //return vec4(noiseOffset, 0, 0, 1);
    //return vec4(normalize(-viewPos) * 0.5 + 0.5, 1.0);

    //vec2 aspect = vec2(_Resolution.y / _Resolution.x, 1.0);
    vec2 aspect = screenSize.yx / screenSize.x;
    float projScale = (-_Radius * projection[0][0]) / viewPos.z;

    //[loop]
    for(int i = 0; i < _SliceCount; i++){
        //float angle = (i + noise + _TemporalDirection) * PI / _SliceCount;
        float angle = (i + noiseDirection + _TemporalDirection) * (PI / _SliceCount);

        vec2 dir = vec2(cos(angle), sin(angle));
        vec2 texel = dir / _Resolution.xy;
        //vec2 texel = (dir / _Resolution.xy) * projScale * aspect;
        //vec2 texel = dir * (1.0 / min(_Resolution.x, _Resolution.y));

        vec3 planeNormal = normalize(cross(vec3(dir,0), viewDir));
        vec3 tangent = cross(viewDir, planeNormal);

        vec3 projNormal = normal - planeNormal * dot(normal, planeNormal);
        vec3 projNorm = normalize(projNormal);

        float cosN = clamp(dot(projNorm, viewDir), -1, 1);
        float n = -sign(dot(projNormal, tangent)) * acos(cosN);

        uint mask = 0;

        color += HorizonSampling(1, _Radius, viewPos, texel, initialStep, uv, viewDir, normal, n, mask);
        color += HorizonSampling(0, _Radius, viewPos, texel, initialStep, uv, viewDir, normal, n, mask);

        ao += float(CountBits(mask)) / 32.0;
    }

    ao /= _SliceCount;
    ao = saturate( pow(1 - saturate(ao), _AOIntensity) );

    color /= _SliceCount;
    color *= _GIIntensity;

    return vec4(color, ao);
}

void main(){
    FragColor = ComputeSSGI(vUV);
}

#endif
