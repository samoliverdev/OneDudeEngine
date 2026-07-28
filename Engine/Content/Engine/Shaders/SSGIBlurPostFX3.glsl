// Edge-aware bilateral blur (SSGI denoise)

#pragma BeginPassDef
    Name Pass0
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name Pass1
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

Texture2D(0, 0, mainTex, mainTexSampler)
Texture2D(0, 1, gNormal, gNormalSampler)
Texture2D(0, 2, gDepth, gDepthSampler)

BeginUniform(0, 0, Main)
    Uniform vec2 giTexelSize;
    Uniform float cameraNear;
    Uniform float cameraFar;

    // tuning params
    Uniform float depthPhi;    // ~100
    Uniform float normalPhi;   // ~32
EndUniform()

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;

out vec2 texCoord;

void main() {
    texCoord = _texCoord;
    gl_Position = vec4(_pos, 1.0);
}
#endif

#if defined(FRAGMENT)

in vec2 texCoord;
out vec4 fragColor;

// --- helpers ---

float LinearizeDepth(float d) {
    float z = d * 2.0 - 1.0;
    return (2.0 * cameraNear * cameraFar) /
           (cameraFar + cameraNear - z * (cameraFar - cameraNear));
}

float GetDepth(vec2 uv){
    return LinearizeDepth(texture(gDepth, uv).r);
}

vec3 GetNormal(vec2 uv){
    return normalize(texture(gNormal, uv).xyz * 2.0 - 1.0);
}

vec3 SampleGI(vec2 uv){
    return texture(mainTex, uv).rgb;
}

// --- bilateral blur core ---

vec3 Blur(vec2 uv, vec2 direction){

    vec3 centerColor = SampleGI(uv);
    float centerDepth = GetDepth(uv);
    vec3 centerNormal = GetNormal(uv);

    vec3 result = vec3(0.0);
    float totalWeight = 0.0;

    const int KERNEL = 2;

    for(int i = -KERNEL; i <= KERNEL; i++){

        vec2 offset = direction * float(i) * giTexelSize;
        vec2 sampleUV = uv + offset;

        vec3 sampleColor = SampleGI(sampleUV);
        float sampleDepth = GetDepth(sampleUV);
        vec3 sampleNormal = GetNormal(sampleUV);

        // spatial weight (gaussian-like)
        float wSpatial = exp(-float(i*i) * 0.5);

        // depth weight
        float depthDiff = abs(centerDepth - sampleDepth);
        float wDepth = exp(-depthDiff * depthPhi);

        // normal weight
        float nDot = max(dot(centerNormal, sampleNormal), 0.0);
        float wNormal = pow(nDot, normalPhi);

        float weight = wSpatial * wDepth * wNormal;

        result += sampleColor * weight;
        totalWeight += weight;
    }

    return result / max(totalWeight, 1e-5);
}

#if defined(Pass0)
void main(){
    // horizontal
    vec2 dir = vec2(1.0, 0.0);
    fragColor = vec4(Blur(texCoord, dir), 1.0);
}
#endif

#if defined(Pass1)
void main(){
    // vertical
    vec2 dir = vec2(0.0, 1.0);
    fragColor = vec4(Blur(texCoord, dir), 1.0);
}
#endif

#endif