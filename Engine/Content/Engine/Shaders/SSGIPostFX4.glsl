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

vec2 ProjectToUV(vec3 viewPos){
    vec4 clip = projection * vec4(viewPos, 1.0);
    vec3 ndc = clip.xyz / clip.w;
    return ndc.xy * 0.5 + 0.5;
}

vec3 CosineSampleHemisphere(vec2 xi){
    float phi = 2.0 * 3.14159265 * xi.x;
    float cosTheta = sqrt(1.0 - xi.y);
    float sinTheta = sqrt(xi.y);

    return vec3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );
}

mat3 CreateTBN(vec3 N){
    vec3 up = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);
    return mat3(T, B, N);
}

vec3 ComputeSSGI(vec2 uv){
    vec2 uResolution = screenSize;
    float uNumSamples = sampleCount;
    int uMaxSteps = int(sliceCount);
    float uRadius = sampleRadius;
    float uThickness = hitThickness;
    float uIntensity = giIntensity;

    vec3 viewPos = GetViewPos(uv);
    vec3 normal = GetViewNormal(uv);// normalize(texture(uNormal, uv).xyz * 2.0 - 1.0);

    mat3 TBN = CreateTBN(normal);

    vec3 indirect = vec3(0.0);
    float totalWeight = 0.0;

    // Blue noise rotation
    vec2 noise = texture(noise, uv * uResolution / 128.0).rg;

    for(int i = 0; i < uNumSamples; i++){
        vec2 xi = fract(noise + vec2(float(i) / float(uNumSamples), 0.0));

        vec3 sampleDir = CosineSampleHemisphere(xi);
        sampleDir = TBN * sampleDir;

        float NdotL = max(dot(normal, sampleDir), 0.0);
        if(NdotL <= 0.0) continue;

        vec3 rayPos = viewPos;
        float stepSize = uRadius / float(uMaxSteps);

        for(int step = 0; step < uMaxSteps; step++){
            rayPos += sampleDir * stepSize;

            vec2 rayUV = ProjectToUV(rayPos);

            if(rayUV.x < 0.0 || rayUV.x > 1.0 || rayUV.y < 0.0 || rayUV.y > 1.0){
                break;
            }

            float sceneDepth = texture(gDepth, rayUV).r;
            vec3 scenePos = GetViewPos(rayUV);

            float depthDiff = rayPos.z - scenePos.z;

            // Thickness test
            if(depthDiff > 0.0 && depthDiff < uThickness){
                vec3 hitColor = texture(mainTex, rayUV).rgb;

                float dist = length(scenePos - viewPos);
                float attenuation = 1.0 / (1.0 + dist * dist);

                indirect += hitColor * attenuation * NdotL;
                totalWeight += NdotL;
                break;
            }
        }
    }

    if(totalWeight > 0.0)
        indirect /= totalWeight;

    return indirect * uIntensity;
}

void main(){
    vec3 gi = ComputeSSGI(texCoord);
    fragColor = vec4(gi, 1);
}

#endif
