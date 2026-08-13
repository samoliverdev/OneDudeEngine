BeginProperties
    Texture2D mainTex White
EndProperties

BeginPass
    #pragma Name MainPass

    #include Engine/ShaderLibrary/Base.glsl
    #include Engine/ShaderLibrary/Core.glsl
    #include Engine/ShaderLibrary/Common.glsl

    BeginUniform(0, 0, Main)
        Uniform vec4 color;

        // 1.0 / GI low-resolution texture size.
        // Example: if GI is 960x540:
        // giTexelSize = vec2(1.0 / 960.0, 1.0 / 540.0)
        Uniform vec2 giTexelSize;

        Uniform vec2 screenSize;

        Uniform float nearPlane;
        Uniform float farPlane;
    EndUniform()

    Texture2D(0, 1, mainTex, mainSampler)
    Texture2D(0, 12, giLow, giLowSampler)
    Texture2D(0, 13, gDepth, gDepthSampler)
    Texture2D(0, 7, gNormal, gNormalSampler)

    BeginVertex

    In(0) vec3 vPos;
    In(1) vec2 vTexCoord;

    Out(0) vec3 pos;
    Out(1) vec2 texCoord;

    void main(){
        pos = vPos;

        #if defined(WebGPU_API)
            texCoord = vec2(vTexCoord.x, 1.0 - vTexCoord.y);
        #else
            texCoord = vTexCoord;
        #endif

        OutPosition = vec4(pos, 1.0);
    }

    EndVertex


    BeginFrag

    In(0) vec3 pos;
    In(1) vec2 texCoord;

    Out(0) vec4 fragColor;

    float Linear01Depth(float depth){
        float z = depth * 2.0 - 1.0;
        float linearDepth = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
        return linearDepth / farPlane;
    }

    vec3 GetNormal(vec2 uv){
        return normalize(unpack_normal_octahedron(texture(gNormal, uv).rg));
    }

    // Gaussian spatial weight
    float SpatialWeight(vec2 offset){
        // Sigma ~= 1.5
        float d2 = dot(offset, offset);
        return exp(-d2 * 0.22);
    }

    // Depth similarity
    float DepthWeight(float centerDepth, float sampleDepth){
        float difference = abs(sampleDepth - centerDepth);

        /*
         * Depth threshold becomes slightly larger farther from
         * the camera. This prevents distant geometry from becoming
         * excessively fragmented.
         */
        float threshold = 0.0025 + centerDepth * 0.035;

        /*
         * Gaussian depth rejection.
         *
         * Small difference:
         *     weight ~= 1
         *
         * Large difference:
         *     weight ~= 0
         */
        float x = difference / max(threshold, 1e-5);
        return exp(-x * x * 2.0);
    }


    // Normal similarity
    float NormalWeight(vec3 centerNormal, vec3 sampleNormal){
        float Ndot = max(dot(centerNormal, sampleNormal), 0.0);

        /*
         * Power makes the rejection sharper around edges.
         *
         * 1.0 = soft
         * 8.0 = strong
         */
        return pow(Ndot, 8.0);
    }


    // ------------------------------------------------------------
    // GI sampling
    vec4 SampleGI(vec2 uv, vec3 centerNormal, float centerDepth, vec2 pixelOffset){
        /*
         * GI is half resolution.

         * The UV is aligned to the center of a low-resolution
         * texel rather than simply sampling arbitrary UV positions.
         */
        vec2 lowUV = uv + pixelOffset * giTexelSize;

        /*
         * Clamp so filtering doesn't pull garbage from outside
         * the texture.
         */
        lowUV = clamp(lowUV, giTexelSize * 0.5, vec2(1.0) - giTexelSize * 0.5);

        vec4 giSample = texture(giLow, lowUV);

        /*
         * IMPORTANT:
         *
         * Depth and normal remain FULL RESOLUTION.
         *
         * This is what makes this an edge-aware upsample rather
         * than simply a blur of the half-resolution GI.
         */
        float sampleDepth = Linear01Depth(texture(gDepth, lowUV).r);

        vec3 sampleNormal = GetNormal(lowUV);

        float spatialWeight = SpatialWeight(pixelOffset);

        float depthWeight = DepthWeight(centerDepth, sampleDepth);

        float normalWeight = NormalWeight(centerNormal, sampleNormal);

        float weight = spatialWeight * depthWeight * normalWeight;

        return vec4(giSample.rgb * weight, giSample.a * weight);
    }

    void main(){
        vec2 uv = texCoord;

        // Center pixel
        float centerDepth = Linear01Depth(texture(gDepth, uv).r);

        vec3 centerNormal = GetNormal(uv);


        // Accumulation
        vec3 gi = vec3(0.0);

        float ao = 0.0;

        float weightSum = 0.0;


        // --------------------------------------------------------
        // 4x4 bilateral filter
        //
        // Half-resolution GI means each GI texel covers roughly
        // a 2x2 region of the final image.
        //
        // 4x4 gives substantially better reconstruction than
        // the original 3x3 kernel.
        // --------------------------------------------------------
        for(int y = -1; y <= 2; y++){
            for(int x = -1; x <= 2; x++){
                vec2 pixelOffset =
                    vec2(float(x), float(y));

                /*
                 * Shift by half a texel so the 4x4 pattern is
                 * centered around the current full-resolution
                 * pixel.
                 */
                pixelOffset += vec2(0.5);

                vec2 lowUV = uv + pixelOffset * giTexelSize;

                lowUV = clamp(lowUV, giTexelSize * 0.5, vec2(1.0) - giTexelSize * 0.5);


                // GI
                vec4 giSample = texture(giLow, lowUV);


                // Full-resolution geometry
                float sampleDepth = Linear01Depth(texture(gDepth, lowUV).r);

                vec3 sampleNormal = GetNormal(lowUV);

                // Spatial
                float spatialWeight = SpatialWeight(pixelOffset);


                // Depth
                float depthDifference = abs(sampleDepth - centerDepth);

                /*
                 * Adaptive threshold.

                 * At near depth:
                 *     very strict
                 *
                 * At far depth:
                 *     slightly more tolerant
                 */
                float depthThreshold = 0.0015 + centerDepth * 0.025;

                float depthX = depthDifference / max(depthThreshold, 1e-5);

                float depthWeight = exp(-depthX * depthX * 2.5);

                // Normal
                float normalDot = max(dot(centerNormal, sampleNormal), 0.0);

                float normalWeight = pow(normalDot, 10.0);


                // ------------------------------------------------
                // Final weight
                float weight = spatialWeight * depthWeight * normalWeight;

                gi += giSample.rgb * weight;
                ao += giSample.a * weight;
                weightSum += weight;
            }
        }

        // Normalize
        float invWeight = 1.0 / max(weightSum, 1e-5);

        gi *= invWeight;
        ao *= invWeight;


        // Output
        fragColor = vec4(gi, ao);
    }
    EndFrag
EndPass