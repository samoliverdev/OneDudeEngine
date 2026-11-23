#pragma BeginPassDef
    Name MainPass
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name SSAOBlurHorizontal
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name SSAOBlurVertical
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name SSAOComposite
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name GTAOPass
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name SSAODebug
    CullFace BACK
    DepthTest DISABLE
    DepthMask False
#pragma EndPassDef

#pragma BeginPassDef
    Name SSAOBilateralUpsample
    CullFace BACK
    DepthTest DISABLE
    DepthMask False
#pragma EndPassDef

#pragma BeginPassDef
    Name SSAOGuidedFit
    CullFace BACK
    DepthTest DISABLE
    DepthMask False
#pragma EndPassDef

#pragma BeginPassDef
    Name SSAOGuidedAverage
    CullFace BACK
    DepthTest DISABLE
    DepthMask False
#pragma EndPassDef

#pragma BeginPassDef
    Name SSAOGuidedUpsample
    CullFace BACK
    DepthTest DISABLE
    DepthMask False
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Common.glsl

BeginUniform(0, 0, Main)
    Uniform vec3 viewPos;
    Uniform int lightIndex;
    Uniform float screenWidth; 
    Uniform float screenHeight;

    Uniform float radius;
    Uniform float intensity;
    Uniform float bias;
    Uniform int outputBentNormals;
    Uniform int debugBentNormals;
    Uniform int useSurfacePlane;
EndUniform()

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
    Uniform mat4 invProjection;
    Uniform mat4 invView;
EndUniform()


Texture2D(0, 4, texNoise, texNoiseSampler)
Texture2D(0, 5, mainTex, mainTexSampler)
Texture2D(0, 6, gPosition, gPositionSampler)
Texture2D(0, 7, gNormal, gNormalSampler)
Texture2D(0, 8, gAlbedoSpec, gAlbedoSpecSampler)
Texture2D(0, 9, gEmission, gEmissionSampler)
Texture2D(0, 10, gOther, gOtherSampler)
Texture2D(0, 12, gDepth, gDepthSampler)
Texture2D(0, 15, ssaoTexture, ssaoSampler)
Texture2D(0, 13, guidedSlope, guidedSlopeSampler)
Texture2D(0, 14, guidedIntercept, guidedInterceptSampler)

#if defined(VERTEX) //&& defined(MainPass)
    layout (location = 0) in vec3 _pos;
    layout (location = 1) in vec2 _texCoord;

    out vec3 pos;
    out vec2 texCoord;

    void main() {
        pos = _pos;
        texCoord = _texCoord;
        gl_Position = vec4(pos, 1.0);
    }
#endif

#if defined(FRAGMENT)
    vec2 DepthTexelUV(vec2 uv){
        ivec2 size = textureSize(gDepth, 0);
        ivec2 pixel = clamp(ivec2(uv * vec2(size)), ivec2(0), size - 1);
        return (vec2(pixel) + 0.5) / vec2(size);
    }
#endif

#if defined(FRAGMENT) && (defined(MainPass) || defined(SSAOBilateralUpsample))
    uniform vec4 samples[64];
    uniform vec2 noiseScale;

    vec4 EvaluateAO(vec2 uv, bool snapDepth, int sampleStride, vec2 sampleNoiseScale){
        vec2 surfaceUV = snapDepth ? DepthTexelUV(uv) : uv;
        float depth = texture(gDepth, surfaceUV).r;
        if(snapDepth && depth >= 1.0) return vec4(1.0, 0.0, 0.0, 0.0);

        vec3 worldPosition = reconstructWorldPos(surfaceUV, depth, invProjection, invView);
        vec3 fragPos = (view * vec4(worldPosition, 1)).xyz;
        vec3 worldNormal = unpack_normal_octahedron(texture(gNormal, surfaceUV).rg);
        vec3 normal = normalize(mat3(view) * worldNormal);

        vec3 randomVec = normalize(texture(texNoise, uv * sampleNoiseScale).xyz);
        vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
        vec3 bitangent = cross(normal, tangent);
        mat3 TBN = mat3(tangent, bitangent, normal);

        float linearDepth = -fragPos.z;
        float ao = 0.0;
        vec3 blockedTangent = vec3(0.0);
        const float falloffDistance = 500.0;
        for(int i = 0; i < 64; i += sampleStride){
            vec3 sampleOffset = TBN * samples[i].xyz;// Sample in tangent space
            vec3 samplePos = fragPos + sampleOffset * radius;// Position in view space

            vec4 offset = projection * vec4(samplePos, 1.0);// Project sample position
            offset.xyz /= offset.w;

            vec2 sampleUV = offset.xy * 0.5 + 0.5;
            if(sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)// Don't allow samples outside the screen
                continue;

            vec2 depthUV = snapDepth ? DepthTexelUV(sampleUV) : sampleUV;
            float rawDepth = texture(gDepth, depthUV).r;
            vec3 sampleSurface = (view * vec4(reconstructWorldPos(depthUV, rawDepth, invProjection, invView), 1.0)).xyz;
            float sampleDepth = -samplePos.z;// Unity's "zDist"
            float surfaceDepth = -sampleSurface.z;// Actual geometry depth
            float isInsideRadius = abs(sampleDepth - surfaceDepth) < radius ? 1.0 : 0.0;// Make sure sampled geometry is actually within radius

            if(rawDepth >= 1.0) // Ignore sky
                isInsideRadius = 0.0;

            // Vector from center pixel to sampled geometry
            vec3 v_s2 = sampleSurface - fragPos;
            float dotVal = dot(v_s2, normal) -0.004 * linearDepth; // Alchemy AO

            float a1 = max(dotVal, 0.0);
            float a2 = dot(v_s2, v_s2) + 0.0001;
            
            float occlusion = a1 / a2 * isInsideRadius;
            ao += occlusion;
            if(outputBentNormals != 0){
                vec3 direction = v_s2 * inversesqrt(a2);
                blockedTangent += occlusion * (direction - normal * dot(direction, normal));
            }
        }

        vec3 meanBlockedTangent = blockedTangent / max(ao, 0.0001);
        ao *= radius;

        float falloff = 1.0 - linearDepth / falloffDistance;
        falloff *= falloff;

        ao *= intensity;
        ao *= falloff;
        ao /= float(64 / sampleStride);

        ao = pow(clamp(ao, 0.0, 1.0), 0.6);
        ao = 1 - ao;

        // R stores visibility. GBA stores a signed world-space direction, or zero in scalar mode.
        vec3 bentNormal = vec3(0.0);
        if(outputBentNormals != 0){
            vec3 bentView = normal - 2.0 * (1.0 - ao) * meanBlockedTangent;
            bentNormal = normalize(mat3(invView) * bentView);
        }
        return vec4(ao, bentNormal);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    in vec2 texCoord;
    out vec4 fragColor;

    void main(){
        bool reducedResolution = any(notEqual(ivec2(screenWidth, screenHeight), textureSize(gDepth, 0)));
        fragColor = EvaluateAO(texCoord, reducedResolution, 1, noiseScale);
    }
#endif

#if defined(FRAGMENT) && defined(SSAOBlurHorizontal)
    in vec2 texCoord;
    out vec4 fragColor;

    vec3 GetNormal(vec2 uv){
        return normalize(unpack_normal_octahedron(texture(gNormal, uv).rg));
    }

    void main(){
        vec2 texelSize = 1.0 / vec2(screenWidth, screenHeight);
        vec3 centerNormal = GetNormal(texCoord);
        vec4 centerAO = texture(ssaoTexture, texCoord);

        float w0 = 0.2270270270;
        float w1 = 0.3162162162;
        float w2 = 0.0702702703;

        vec4 ao = centerAO * w0;
        float weight = w0;

        vec2 offset1 = vec2(1.3846153846 * texelSize.x, 0.0);
        vec2 offset2 = vec2(3.2307692308 * texelSize.x, 0.0);

        // ---------------------------------------------------------
        // +1
        // ---------------------------------------------------------

        vec2 uv = texCoord + offset1;
        vec3 normal = GetNormal(uv);
        float normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        vec4 sampleAO = texture(ssaoTexture, uv);
        float w = w1 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // -1
        uv = texCoord - offset1;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv);

        w = w1 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // +2
        uv = texCoord + offset2;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv);

        w = w2 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // -2
        uv = texCoord - offset2;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv);

        w = w2 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        ao /= max(weight, 0.0001);
        fragColor = ao;
    }
#endif

#if defined(FRAGMENT) && defined(SSAOBlurVertical)
    in vec2 texCoord;
    out vec4 fragColor;

    vec3 GetNormal(vec2 uv){
        return normalize(unpack_normal_octahedron(texture(gNormal, uv).rg));
    }

    void main(){
        vec2 texelSize = 1.0 / vec2(screenWidth, screenHeight);

        vec3 centerNormal = GetNormal(texCoord);

        vec4 centerAO = texture(ssaoTexture, texCoord);

        float w0 = 0.2270270270;
        float w1 = 0.3162162162;
        float w2 = 0.0702702703;

        vec4 ao = centerAO * w0;
        float weight = w0;

        vec2 offset1 = vec2(0.0, 1.3846153846 * texelSize.y);
        vec2 offset2 = vec2(0.0, 3.2307692308 * texelSize.y);

        // +1

        vec2 uv = texCoord + offset1;

        vec3 normal = GetNormal(uv);
        float normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        vec4 sampleAO = texture(ssaoTexture, uv);
        float w = w1 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // -1
        uv = texCoord - offset1;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv);

        w = w1 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // +2
        uv = texCoord + offset2;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv);

        w = w2 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // -2
        uv = texCoord - offset2;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv);

        w = w2 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        ao /= max(weight, 0.0001);
        fragColor = ao;
    }
#endif

#if defined(FRAGMENT) && defined(SSAOComposite)
    in vec2 texCoord;
    out vec4 fragColor;

    void main(){
        vec3 color = texture(mainTex, texCoord).rgb;
        float ao = texture(ssaoTexture, texCoord).r;
        fragColor = vec4(color * ao, 1.0);
        //fragColor = vec4(ao.xxx, 1.0);
    }
#endif

#if defined(FRAGMENT) && (defined(SSAOBilateralUpsample) || defined(SSAOGuidedFit) || defined(SSAOGuidedUpsample))
    vec3 ViewPosition(vec2 uv, float depth){
        #ifdef Vulkan_API
        float ndcDepth = depth;
        #else
        float ndcDepth = depth * 2.0 - 1.0;
        #endif
        vec4 viewPosition = invProjection * vec4(uv * 2.0 - 1.0, ndcDepth, 1.0);
        return viewPosition.xyz / viewPosition.w;
    }
#endif

#if defined(FRAGMENT) && defined(SSAOBilateralUpsample)
    in vec2 texCoord;
    out vec4 fragColor;

    float SurfaceWeight(ivec2 tap, ivec2 lowSize, vec3 centerPosition, vec3 centerNormal, vec3 viewNormal, float tolerance){
        vec2 tapUV = DepthTexelUV((vec2(tap) + 0.5) / vec2(lowSize));
        float tapDepth = texture(gDepth, tapUV).r;
        if(tapDepth >= 1.0) return 0.0;

        vec3 tapPosition = ViewPosition(tapUV, tapDepth);
        vec3 tapNormal = unpack_normal_octahedron(texture(gNormal, tapUV).rg);
        vec3 delta = tapPosition - centerPosition;
        float surfaceDistance = abs(useSurfacePlane != 0 ? dot(delta, viewNormal) : delta.z);
        float surfaceWeight = 1.0 - smoothstep(tolerance, 3.0 * tolerance, surfaceDistance);
        float normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, tapNormal));
        return surfaceWeight * normalWeight;
    }

    void main(){
        fragColor = vec4(1.0, 0.0, 0.0, 0.0);
        float rawDepth = texture(gDepth, texCoord).r;
        if(rawDepth >= 1.0) return;

        vec3 centerPosition = ViewPosition(texCoord, rawDepth);
        vec3 centerNormal = unpack_normal_octahedron(texture(gNormal, texCoord).rg);
        vec3 viewNormal = mat3(view) * centerNormal;
        ivec2 lowSize = textureSize(ssaoTexture, 0);
        vec2 lowPosition = texCoord * vec2(lowSize) - 0.5;
        ivec2 lowBase = ivec2(floor(lowPosition));
        vec2 blend = fract(lowPosition);
        float tolerance = useSurfacePlane != 0
            ? max(0.005, -0.001 * centerPosition.z)
            : max(0.01, -0.02 * centerPosition.z);

        vec4 sum = vec4(0.0);
        float totalWeight = 0.0;
        for(int y = 0; y < 2; ++y){
            for(int x = 0; x < 2; ++x){
                ivec2 tap = clamp(lowBase + ivec2(x, y), ivec2(0), lowSize - 1);
                vec2 spatialWeight = mix(1.0 - blend, blend, vec2(x, y));
                float weight = spatialWeight.x * spatialWeight.y *
                    SurfaceWeight(tap, lowSize, centerPosition, centerNormal, viewNormal, tolerance);
                sum += texelFetch(ssaoTexture, tap, 0) * weight;
                totalWeight += weight;
            }
        }

        // A thin edge can miss all four taps. Look for the closest compatible surface.
        if(useSurfacePlane != 0 && totalWeight < 0.05){
            ivec2 nearest = ivec2(floor(lowPosition + 0.5));
            float bestWeight = 0.0;
            vec4 bestAO = fragColor;
            for(int y = -1; y <= 1; ++y){
                for(int x = -1; x <= 1; ++x){
                    ivec2 tap = clamp(nearest + ivec2(x, y), ivec2(0), lowSize - 1);
                    vec2 offset = vec2(tap) - lowPosition;
                    float weight = SurfaceWeight(tap, lowSize, centerPosition, centerNormal, viewNormal, tolerance) /
                        (1.0 + dot(offset, offset));
                    if(weight > bestWeight){
                        bestWeight = weight;
                        bestAO = texelFetch(ssaoTexture, tap, 0);
                    }
                }
            }
            sum += bestAO * bestWeight;
            totalWeight += bestWeight;
        }

        if(totalWeight >= 0.05){
            fragColor = sum / totalWeight;
            return;
        }

        // Low-resolution taps can miss thin geometry entirely. Evaluate the receiving pixel.
        vec4 edgeAO = EvaluateAO(texCoord, true, 2, vec2(textureSize(gDepth, 0)) * 0.5);
        if(totalWeight > 0.0001){
            edgeAO = mix(edgeAO, sum / totalWeight, smoothstep(0.0, 0.05, totalWeight));
        }
        fragColor = edgeAO;
    }
#endif

#if defined(FRAGMENT) && (defined(SSAOGuidedFit) || defined(SSAOGuidedUpsample))
    float GuidedDepth(vec2 uv, float depth){
        // Log depth makes regularization respond to relative depth changes.
        return log2(max(-ViewPosition(uv, min(depth, 0.999999)).z, 0.0001));
    }
#endif

#if defined(FRAGMENT) && defined(SSAOGuidedFit)
    layout(location = 0) out vec4 slope;
    layout(location = 1) out vec4 intercept;

    void main(){
        ivec2 size = textureSize(ssaoTexture, 0);
        ivec2 center = ivec2(gl_FragCoord.xy);
        vec2 centerUV = DepthTexelUV((vec2(center) + 0.5) / vec2(size));
        float centerDepth = GuidedDepth(centerUV, texture(gDepth, centerUV).r);
        float meanDepth = 0.0;
        float meanDepthSquared = 0.0;
        vec4 meanAO = vec4(0.0);
        vec4 meanDepthAO = vec4(0.0);

        for(int y = -1; y <= 1; ++y){
            for(int x = -1; x <= 1; ++x){
                ivec2 tap = clamp(center + ivec2(x, y), ivec2(0), size - 1);
                vec2 uv = DepthTexelUV((vec2(tap) + 0.5) / vec2(size));
                float rawDepth = texture(gDepth, uv).r;
                float depth = GuidedDepth(uv, rawDepth) - centerDepth;
                vec4 ao = rawDepth < 1.0 ? texelFetch(ssaoTexture, tap, 0) : vec4(1.0, 0.0, 0.0, 0.0);
                meanDepth += depth;
                meanDepthSquared += depth * depth;
                meanAO += ao;
                meanDepthAO += depth * ao;
            }
        }

        meanDepth /= 9.0;
        meanDepthSquared /= 9.0;
        meanAO /= 9.0;
        meanDepthAO /= 9.0;
        float variance = max(meanDepthSquared - meanDepth * meanDepth, 0.0);
        // Dampen shallow depth gradients so the model does not amplify quantization.
        slope = (meanDepthAO - meanDepth * meanAO) / (variance + 0.0025);
        intercept = meanAO - slope * (meanDepth + centerDepth);
    }
#endif

#if defined(FRAGMENT) && defined(SSAOGuidedAverage)
    layout(location = 0) out vec4 meanSlope;
    layout(location = 1) out vec4 meanIntercept;

    void main(){
        ivec2 size = textureSize(guidedSlope, 0);
        ivec2 center = ivec2(gl_FragCoord.xy);
        meanSlope = vec4(0.0);
        meanIntercept = vec4(0.0);
        for(int y = -1; y <= 1; ++y){
            for(int x = -1; x <= 1; ++x){
                ivec2 tap = clamp(center + ivec2(x, y), ivec2(0), size - 1);
                meanSlope += texelFetch(guidedSlope, tap, 0);
                meanIntercept += texelFetch(guidedIntercept, tap, 0);
            }
        }
        meanSlope /= 9.0;
        meanIntercept /= 9.0;
    }
#endif

#if defined(FRAGMENT) && defined(SSAOGuidedUpsample)
    in vec2 texCoord;
    out vec4 fragColor;

    void main(){
        float rawDepth = texture(gDepth, texCoord).r;
        if(rawDepth >= 1.0){
            fragColor = vec4(1.0, 0.0, 0.0, 0.0);
            return;
        }

        // Interpolate the fitted models, then evaluate against full-resolution depth.
        vec2 edge = 0.5 / vec2(textureSize(guidedSlope, 0));
        vec2 uv = clamp(texCoord, edge, 1.0 - edge);
        vec4 slope = texture(guidedSlope, uv);
        vec4 intercept = texture(guidedIntercept, uv);
        vec4 ao = slope * GuidedDepth(texCoord, rawDepth) + intercept;
        fragColor = clamp(ao, vec4(0.0, -1.0, -1.0, -1.0), vec4(1.0));
    }
#endif

#if defined(FRAGMENT) && defined(SSAODebug)
    in vec2 texCoord;
    out vec4 fragColor;

    void main(){
        if(texture(gDepth, texCoord).r >= 1.0){
            fragColor = vec4(vec3(debugBentNormals == 0 ? 1.0 : 0.0), 1.0);
            return;
        }

        vec4 ao = texture(ssaoTexture, texCoord);
        if(debugBentNormals == 0){
            fragColor = vec4(ao.rrr, 1.0);
        } else {
            vec3 bentNormal = ao.gba;
            float lengthSquared = dot(bentNormal, bentNormal);
            if(lengthSquared > 0.0001) bentNormal *= inversesqrt(lengthSquared);
            fragColor = vec4(bentNormal * 0.5 + 0.5, 1.0);
        }
    }
#endif

#if defined(FRAGMENT) && defined(GTAOPass)
    in vec2 texCoord;

    out vec4 fragColor;

    const int slices = 8; //directions swept per pixel
    const int stepsPerSlice = 8; //horizon steps per side, per slice
    const float falloffRange = 0.4*1; //fraction of radius samples fade out over
    const float power = 1.5; //artistic control! oh no!
    const float PI = 3.14159265358979;
    const float HALF_PI = 1.57079632679;

    vec3 GetViewPosition(vec2 uv){
        float depth = texture(gDepth, uv).r;
        vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
        vec4 view = invProjection * clip;
        view /= view.w;
        return view.xyz;

        /*float depth = texture(gDepth, uv).r;
        vec3 worldPos = reconstructWorldPos(uv, depth, invProjection, invView);
        return (view * vec4(worldPos, 1.0)).xyz;*/
    }

    vec3 GetViewNormal(vec2 uv){
        vec3 normal = unpack_normal_octahedron(texture(gNormal, uv).rg);
        normal = mat3(view) * normal;
        return normalize(normal);
    }

    float integrateSlice(float n, float cosNorm, float h0, float h1){
        float iarc0 = (cosNorm + 2.0 * h0 * sin(n) - cos(2.0 * h0 - n)) / 4.0;
        float iarc1 = (cosNorm + 2.0 * h1 * sin(n) - cos(2.0 * h1 - n)) / 4.0;
        return iarc0 + iarc1;
    }

    void main(){
        vec2 SCREEN_UV = texCoord;
        vec2 VIEWPORT_SIZE = vec2(screenWidth, screenHeight);

        vec3 C = GetViewPosition(SCREEN_UV);

        vec3 normal = GetViewNormal(SCREEN_UV); //texture(normalTexture, SCREEN_UV).xyz;
        //normal = normalize(normal * 2.0 - 1.0); //view space normal

        vec3 viewVec = normalize(-C);

        //ivec2 texel = ivec2(mod(floor(SCREEN_UV * push_constants.VIEWPORT_SIZE), 4.0));
        vec2 noiseVec = texture(texNoise, texCoord*150).xy; //vec2(0, 0);// texelFetch(noiseTexture, texel, 0).xy;
        float noiseSlice = noiseVec.x;
        float noiseStep = noiseVec.y;

        float projScale = abs(projection[1][1]) * VIEWPORT_SIZE.y * 0.5;
        float screenspaceRadius = projScale * radius / max(-C.z, 0.001);

        float falloffFrom = radius * (1.0 - falloffRange);
        float falloffRangeInv = falloffRange * radius;
        float falloffMul = -1.0 / falloffRangeInv;
        float falloffAdd = falloffFrom / falloffRangeInv + 1.0;

        float visibility = 0.0;

        for(int slice = 0; slice < slices; slice++){
            float sliceK = (float(slice) + noiseSlice) / float(slices);
            float phi = sliceK * PI;
            float cosPhi = cos(phi);
            float sinPhi = sin(phi);
            vec2 omega = vec2(cosPhi, -sinPhi) * screenspaceRadius;

            vec3 directionVec = vec3(cosPhi, sinPhi, 0.0);
            vec3 orthoDirectionVec = directionVec - dot(directionVec, viewVec) * viewVec;
            vec3 axisVec = normalize(cross(orthoDirectionVec, viewVec));
            vec3 projectedNormalVec = normal - axisVec * dot(normal, axisVec);
            float projectedNormalVecLength = length(projectedNormalVec);

            float signNorm = sign(dot(orthoDirectionVec, projectedNormalVec));
            float cosNorm = clamp(dot(projectedNormalVec, viewVec) / max(projectedNormalVecLength, 0.0001), -1.0, 1.0);
            float n = signNorm * acos(cosNorm);

            float lowHorizonCos0 = cos(n + HALF_PI);
            float lowHorizonCos1 = cos(n - HALF_PI);
            float horizonCos0 = lowHorizonCos0;
            float horizonCos1 = lowHorizonCos1;

            for(int step = 0; step < stepsPerSlice; step++){
                float stepNoise = fract(noiseStep + float(step) * 0.6180339887);
                float s = (float(step) + stepNoise) / float(stepsPerSlice);

                vec2 uvOffset = (s * omega) / VIEWPORT_SIZE;

                vec2 sampleUV0 = SCREEN_UV + uvOffset;
                vec2 sampleUV1 = SCREEN_UV - uvOffset;

                vec3 samplePos0 = GetViewPosition(sampleUV0);
                vec3 samplePos1 = GetViewPosition(sampleUV1);

                if(sampleUV0.x < 0.0 || sampleUV0.x > 1.0 || sampleUV0.y < 0.0 || sampleUV0.y > 1.0){
                    continue;
                }

                if(sampleUV1.x < 0.0 || sampleUV1.x > 1.0 || sampleUV1.y < 0.0 || sampleUV1.y > 1.0){
                    continue;
                }

                vec3 delta0 = samplePos0 - C;
                vec3 delta1 = samplePos1 - C;
                float dist0 = length(delta0);
                float dist1 = length(delta1);

                vec3 horizonVec0 = delta0 / max(dist0, 0.0001);
                vec3 horizonVec1 = delta1 / max(dist1, 0.0001);

                float weight0 = clamp(dist0 * falloffMul + falloffAdd, 0.0, 1.0);
                float weight1 = clamp(dist1 * falloffMul + falloffAdd, 0.0, 1.0);

                float shc0 = mix(lowHorizonCos0, dot(horizonVec0, viewVec), weight0);
                float shc1 = mix(lowHorizonCos1, dot(horizonVec1, viewVec), weight1);

                horizonCos0 = max(horizonCos0, shc0);
                horizonCos1 = max(horizonCos1, shc1);
            }

            float h0 = -acos(horizonCos1);
            float h1 = acos(horizonCos0);

            visibility += projectedNormalVecLength * integrateSlice(n, cosNorm, h0, h1);
        }

        visibility /= float(slices);
        float ao = pow(clamp(visibility, 0.0, 1.0), power);

        fragColor = vec4(ao, ao, ao, 1.0);
    }
#endif