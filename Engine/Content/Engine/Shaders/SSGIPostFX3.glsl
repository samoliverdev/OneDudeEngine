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
EndUniform()

// ============================================================
// VERTEX
// ============================================================
#if defined(VERTEX) && defined(MainPass)

layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;

out vec2 texCoord;

void main(){
    texCoord = _texCoord;
    gl_Position = vec4(_pos, 1.0);
}

#endif

// ============================================================
// FRAGMENT
// ============================================================
#if defined(FRAGMENT) && defined(MainPass)

in vec2 texCoord;
out vec4 fragColor;

#include Engine/ShaderLibrary/Vertex.glsl

// ===== Constants =====
const float PI      = 3.14159265359;
const float HALF_PI = 1.57079632679;
const float TWO_PI  = 6.28318530718;

// ===== Tunables =====
const uint  SECTOR_COUNT = 32u;
const float AO_POWER     = 1.5;
const float EXP_FACTOR   = 2.0;
const float GI_CLAMP     = 7.0;

uint BitCount(uint v){
    return uint(bitCount(v));
}

// ============================================================
// Fast acos (Activision GTAO)
// ============================================================
float fastAcos(float x){
    float y = abs(x);
    float p = -0.156583 * y + HALF_PI;
    p *= sqrt(max(1.0 - y, 0.0));
    return (x >= 0.0) ? p : PI - p;
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

// ============================================================
// Geometry helpers
// ============================================================
vec3 GetViewPos(vec2 uv){
    vec3 wp = reconstructWorldPos(
        uv,
        texture(gDepth, uv).r,
        invProjection,
        invView
    );
    return (view * vec4(wp, 1.0)).xyz;
}

vec3 GetViewNormal(vec2 uv){
    vec3 n = unpack_normal_octahedron(texture(gNormal, uv).rg);
    return normalize(mat3(view) * n);
}

// ============================================================
// Bitmask update (GLSL-safe)
// ============================================================
uint UpdateMask(float h0, float h1, uint mask){
    uint startBit = clamp(
        uint(h0 * float(SECTOR_COUNT)),
        0u,
        SECTOR_COUNT - 1u
    );

    uint endBit = clamp(
        uint(h1 * float(SECTOR_COUNT)),
        0u,
        SECTOR_COUNT
    );

    if(endBit <= startBit)
        return mask;

    uint count = endBit - startBit;

    uint bits = ((1u << count) - 1u) << startBit;
    return mask | bits;
}

// ============================================================
// Exponential stepping
// ============================================================
float ExpStep(float i, float n, float jitter){
    float t = (i + jitter) / n;
    return pow(t, EXP_FACTOR);
}

// ============================================================
// Main
// ============================================================
void main(){
    vec3 P = GetViewPos(texCoord);
    vec3 N = GetViewNormal(texCoord);
    vec3 V = normalize(-P);

    float aoAccum = 0.0;
    vec3  giAccum = vec3(0.0);

    vec2 aspect = vec2(screenSize.y / screenSize.x, 1.0);
    float projScale = (-sampleRadius * projection[0][0]) / P.z;

    float pixelJitter = blueNoise(texCoord);

    uint sliceCnt = uint(sliceCount);
    uint sampCnt  = uint(sampleCount);

    for(uint s = 0u; s < sliceCnt; ++s){
        float sliceJitter = randf(gl_FragCoord.xy + float(s) * 17.0);
        float phi = (float(s) + sliceJitter) * (TWO_PI / sliceCount);
        vec2  dir = vec2(cos(phi), sin(phi));

        uint aoMask = 0u;
        uint giMask = 0u;

        for(uint side = 0u; side < 2u; ++side){
            vec2 d = (side == 0u) ? dir : -dir;

            for(uint i = 0u; i < sampCnt; ++i){

                float stepT = ExpStep(float(i), sampleCount, pixelJitter);
                float step  = stepT * sampleRadius + 0.01;

                vec2 uv = texCoord - d * step * projScale * aspect;

                if(any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) break;

                vec3 Sf = GetViewPos(uv);
                vec3 Sb = Sf - V * hitThickness *
                          clamp(abs(P.z) * 0.02, 1.0, 5.0);

                vec3 Lf = normalize(Sf - P);
                vec3 Lb = normalize(Sb - P);

                float tf = fastAcos(dot(Lf, V));
                float tb = fastAcos(dot(Lb, V));

                float h0 = min(tf, tb);
                float h1 = max(tf, tb);

                // Slice-space normal
                vec3 sliceT = normalize(vec3(d, 0.0));
                vec3 axis   = cross(sliceT, V);
                vec3 Np     = normalize(N - axis * dot(N, axis));

                float signN = sign(dot(sliceT, Np));
                float cosN  = clamp(dot(Np, V), -1.0, 1.0);
                float nAng  = signN * fastAcos(cosN);

                // Normalize to angular space [0,1]
                h0 = clamp((h0 + nAng + HALF_PI) / PI, 0.0, 1.0);
                h1 = clamp((h1 + nAng + HALF_PI) / PI, 0.0, 1.0);

                // Half-sector rule
                float sectorSize = 1.0 / float(SECTOR_COUNT);
                h0 += 0.5 * sectorSize;
                h1 -= 0.5 * sectorSize;

                if(h1 <= h0) continue;

                uint mask = UpdateMask(h0, h1, 0u);

                // ===== GI =====
                uint newlyVisible = mask & ~giMask;
                uint visCount     = BitCount(newlyVisible);

                if(visCount > 0u){
                    vec3 Ns = GetViewNormal(uv);
                    vec3 Ld = normalize(Sf - P);

                    float ndl = max(dot(N,  Ld), 0.0);
                    float ldn = max(dot(Ns, -Ld), 0.0);

                    float dist    = length(Sf - P);
                    float falloff = 1.0 / (1.0 + dist * dist * 0.1);

                    vec3 Li = texture(mainTex, uv).rgb;

                    giAccum += (float(visCount) / float(SECTOR_COUNT)) * Li * ndl * ldn * falloff;
                }

                giMask |= mask;
                aoMask |= mask;
            }
        }

        aoAccum += 1.0 - float(BitCount(aoMask)) / float(SECTOR_COUNT);
    }

    float AO = pow(
        clamp(aoAccum / sliceCount, 0.0, 1.0),
        AO_POWER
    );

    vec3 GI = giAccum / sliceCount;

    // HDR clamp
    float lum = dot(GI, vec3(0.2126, 0.7152, 0.0722));
    if(lum > GI_CLAMP) GI *= GI_CLAMP / lum;

    /*vec3 currGI = GI;
    vec3 prevGI = texture(lastIndirect, texCoord).rgb;
    float alpha = 0.1; // ~10 frame convergence
    vec3 minGI = min(currGI, prevGI);// Clamp history (prevents ghosting)
    vec3 maxGI = max(currGI, prevGI);
    prevGI = clamp(prevGI, minGI, maxGI);
    GI = mix(currGI, prevGI, alpha);*/

    //fragColor = vec4((GI * giIntensity) * AO, AO);
    fragColor = vec4((GI * giIntensity), AO);
    return;

    vec4 directLighting = texture(mainTex, texCoord);
    vec4 diffuse = texture(gAlbedoSpec, texCoord);
    fragColor = vec4((directLighting.rgb * AO) + (diffuse.rgb * (GI*giIntensity)), directLighting.a);
    
    //vec3 directLighting = texture(mainTex, texCoord).rgb; // Direct lighting
    //fragColor = vec4((directLighting + (GI * giIntensity) * AO), AO);
    //fragColor = vec4(N, AO);
}

#endif
