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
Texture2D(0, 4, ssaoTexture, ssaoSampler)

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

#if defined(FRAGMENT) && defined(MainPass)
    in vec3 pos;
    in vec2 texCoord;
    out vec4 fragColor;

    uniform vec4 samples[64]; // Sample kernel

    /*uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 invProjection;
    uniform mat4 invView;*/

    /*uniform float intensity = 0.5;
    uniform float radius = 0.5;
    uniform float bias = 0.025;*/
    int kernelSize = 64;

    uniform vec2 noiseScale;

    float rand(vec2 co) {
        return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
    }

    void main(){
        float depth = texture(gDepth, texCoord).r;
        //if(depth >= 1.0) discard;

        vec3 fragPos = reconstructWorldPos(texCoord, texture(gDepth, texCoord).r, invProjection, invView); //texture(gPosition, texCoord).rgb;
        vec3 normal = unpack_normal_octahedron(texture(gNormal, texCoord).rg); //texture(gNormal, texCoord).rgb;
        
        fragColor = vec4(normal, 1);
        //return;

        vec3 randomVec = normalize(texture(texNoise, texCoord * noiseScale).xyz);
        fragColor = vec4(randomVec, 1);
        //return;

        /*vec2 noiseSeed = gl_FragCoord.xy / screenSize;
        randomVec = normalize(vec3(
            rand(noiseSeed),
            rand(noiseSeed + vec2(1.0, 0.0)),
            rand(noiseSeed + vec2(0.0, 1.0))
        ) * 2.0 - 1.0);*/

        fragPos = (view * vec4(fragPos, 1)).xyz;
        normal = normalize(mat3(view) * normal);  // Or use normal matrix //normalize((view * vec4(normal, 0)).xyz);

        //fragColor = vec4(normal, 1);
        //return;

        vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
        vec3 bitangent = cross(normal, tangent);
        mat3 TBN = mat3(tangent, bitangent, normal);
    
        /*float occlusion = 0.0;
        for(int i = 0; i < kernelSize; ++i){
            // get sample position
            vec3 samplePos = TBN * samples[i].xyz; // from tangent to view-space
            samplePos = fragPos + samplePos * radius; 
            
            // project sample position (to sample texture) (to get position on screen/texture)
            vec4 offset = vec4(samplePos, 1.0);
            offset = projection * offset; // from view to clip-space
            offset.xyz /= offset.w; // perspective divide
            offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
            
            // get sample depth
            //float sampleDepth = (view * vec4(texture(gPosition, offset.xy).xyz, 1)).z; //texture(gPosition, offset.xy).z; // get depth value of kernel sample
            float sampleDepth = (view * vec4(reconstructWorldPos(offset.xy, texture(gDepth, offset.xy).r, invProjection, invView), 1)).z;
            
            // range check & accumulate
            float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
            occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;           
        }
        occlusion = 1.0 - (occlusion / float(kernelSize));

        if(depth >= 1) occlusion = 1;

        occlusion = pow(occlusion, intensity);
        fragColor = vec4(occlusion, occlusion, occlusion, 1);*/

        float linearDepth = -fragPos.z;
        float ao = 0.0;
        const float falloffDistance = 500.0;
        const float beta = 0.004;
        const float epsilon = 0.0001;
        for(int i = 0; i < kernelSize; ++i){
            vec3 sampleOffset = TBN * samples[i].xyz;// Sample in tangent space
            vec3 samplePos = fragPos + sampleOffset * radius;// Position in view space

            vec4 offset = projection * vec4(samplePos, 1.0);// Project sample position
            offset.xyz /= offset.w;

            vec2 sampleUV = offset.xy * 0.5 + 0.5;
            if(sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)// Don't allow samples outside the screen
                continue;

            float rawDepth = texture(gDepth, sampleUV).r;// Depth buffer at projected position
            vec3 sampleSurface = (view * vec4(reconstructWorldPos(sampleUV, rawDepth, invProjection, invView), 1.0)).xyz;// Reconstruct view position
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
            
            ao += a1 / a2 * isInsideRadius;
        }

        ao *= radius;

        float falloff = 1.0 - linearDepth / falloffDistance;
        falloff *= falloff;

        ao *= intensity;
        ao *= falloff;
        ao /= float(kernelSize);

        ao = pow(clamp(ao, 0.0, 1.0), 0.6);
        ao = 1 - ao;

        fragColor = vec4(ao, ao, ao, 1);
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
        float centerAO = texture(ssaoTexture, texCoord).r;

        float w0 = 0.2270270270;
        float w1 = 0.3162162162;
        float w2 = 0.0702702703;

        float ao = centerAO * w0;
        float weight = w0;

        vec2 offset1 = vec2(1.3846153846 * texelSize.x, 0.0);
        vec2 offset2 = vec2(3.2307692308 * texelSize.x, 0.0);

        // ---------------------------------------------------------
        // +1
        // ---------------------------------------------------------

        vec2 uv = texCoord + offset1;
        vec3 normal = GetNormal(uv);
        float normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        float sampleAO = texture(ssaoTexture, uv).r;
        float w = w1 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // -1
        uv = texCoord - offset1;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv).r;

        w = w1 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // +2
        uv = texCoord + offset2;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv).r;

        w = w2 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // -2
        uv = texCoord - offset2;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv).r;

        w = w2 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        ao /= max(weight, 0.0001);
        fragColor = vec4(ao, ao, ao, 1.0);
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

        float centerAO = texture(ssaoTexture, texCoord).r;

        float w0 = 0.2270270270;
        float w1 = 0.3162162162;
        float w2 = 0.0702702703;

        float ao = centerAO * w0;
        float weight = w0;

        vec2 offset1 = vec2(0.0, 1.3846153846 * texelSize.y);
        vec2 offset2 = vec2(0.0, 3.2307692308 * texelSize.y);

        // +1

        vec2 uv = texCoord + offset1;

        vec3 normal = GetNormal(uv);
        float normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        float sampleAO = texture(ssaoTexture, uv).r;
        float w = w1 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // -1
        uv = texCoord - offset1;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv).r;

        w = w1 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // +2
        uv = texCoord + offset2;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv).r;

        w = w2 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        // -2
        uv = texCoord - offset2;
        normal = GetNormal(uv);
        normalWeight = smoothstep(0.6, 1.0, dot(centerNormal, normal));
        sampleAO = texture(ssaoTexture, uv).r;

        w = w2 * normalWeight;

        ao += sampleAO * w;
        weight += w;

        ao /= max(weight, 0.0001);
        fragColor = vec4(ao, ao, ao, 1.0);
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