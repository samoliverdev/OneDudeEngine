#pragma BeginPassDef
    Name NormalDepthDownsample
#pragma EndPassDef

#pragma BeginPassDef
    Name UpSample
#pragma EndPassDef

#pragma BeginPassDef
    Name UpSample2
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Core.glsl
#include Engine/ShaderLibrary/Common.glsl

BeginUniform(0, 0, Main)
        //Uniform vec2 lowScreenSize;

        // Output GI resolution.
        //
        // Half:
        //     screenSize * 0.5
        //
        // Quarter:
        //     screenSize * 0.25
        //Uniform vec2 giSize;

        Uniform float nearPlane;
        Uniform float farPlane;

        // --------------------------------------------------------
        // Surface selection
        // --------------------------------------------------------

        // Depth similarity.
        //
        // Higher = prefer samples with similar depth.
        //Uniform float depthSigma;

        // Normal similarity.
        //
        // Higher = prefer samples with similar normals.
        Uniform float normalSigma;

        // Spatial preference.
        //
        // Higher = prefer samples near the low-res pixel center.
        //Uniform float spatialSigma;

        // --------------------------------------------------------
        // Full-resolution screen
        // --------------------------------------------------------

        Uniform vec2 screenSize;

        // --------------------------------------------------------
        // GI resolution
        // --------------------------------------------------------

        Uniform vec2 giSize;
        Uniform vec2 giTexelSize;


        // --------------------------------------------------------
        // Bilateral parameters
        // --------------------------------------------------------
        Uniform float spatialSigma;
        Uniform float depthSigma;
        Uniform float normalPower;

EndUniform()
// ------------------------------------------------------------
// Full-resolution GBuffer
// ------------------------------------------------------------

Texture2D(0, 13, gDepth, gDepthSampler)
Texture2D(0, 7,  gNormal, gNormalSampler)

// ============================================================
// Textures
// ============================================================

// Low-resolution SSGI + AO.
//
// RGB = SSGI
// A   = AO
Texture2D(
    0,
    12,
    giLow,
    giLowSampler
);

// Surface-aware low-resolution geometry.
//
// RGB = representative normal
// A   = representative linear depth
Texture2D(
    0,
    14,
    giSurface,
    giSurfaceSampler
);

#if defined(VERTEX)
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
#endif

#if defined(FRAGMENT)
    In(0) vec3 pos;
    In(1) vec2 texCoord;
    Out(0) vec4 fragColor;

    #include Engine/ShaderLibrary/Vertex.glsl

    // ============================================================
    // Depth
    // ============================================================

    float Linear01Depth(float depth)
    {
        float z =
            depth * 2.0 - 1.0;

        float linearDepth =
            (
                2.0 *
                nearPlane *
                farPlane
            )
            /
            (
                farPlane +
                nearPlane -
                z *
                (
                    farPlane -
                    nearPlane
                )
            );

        return linearDepth / farPlane;
    }


    // ============================================================
    // Normal
    // ============================================================

    vec3 GetNormal(vec2 uv)
    {
        return normalize(
            unpack_normal_octahedron(
                texture(
                    gNormal,
                    uv
                ).rg
            )
        );
    }

    #if defined(NormalDepthDownsample)
    void main(){
        // --------------------------------------------------------
        // Full-resolution texel size
        // --------------------------------------------------------

        vec2 screenTexelSize =
            1.0 / screenSize;


        // --------------------------------------------------------
        // Low-res pixel center
        // --------------------------------------------------------
        //
        // texCoord is already the center of the current output
        // pixel when rendering a fullscreen triangle/quad.
        //
        // We use this UV as the center of the surface search.
        // --------------------------------------------------------

        vec2 centerUV =
            texCoord;


        // --------------------------------------------------------
        // Center surface
        // --------------------------------------------------------

        float centerDepth =
            Linear01Depth(
                texture(
                    gDepth,
                    centerUV
                ).r
            );

        vec3 centerNormal =
            GetNormal(centerUV);


        // --------------------------------------------------------
        // Determine downsample ratio.
        //
        // Half:
        //     1920 / 960 = 2
        //
        // Quarter:
        //     1920 / 480 = 4
        // --------------------------------------------------------

        vec2 scale =
            screenSize / giSize;


        // We expect the X/Y scale to normally be equal.
        //
        // Use X for determining the search radius.
        float scaleX = scale.x;


        // --------------------------------------------------------
        // Search size
        //
        // Half    -> 2x2
        // Quarter -> 4x4
        //
        // This assumes integer downsample factors.
        // --------------------------------------------------------

        int searchSize =
            int(
                floor(
                    scaleX + 0.5
                )
            );

        searchSize =
            max(
                searchSize,
                1
            );


        // --------------------------------------------------------
        // Accumulation
        // --------------------------------------------------------

        float bestScore =
            1e30;

        float bestDepth =
            centerDepth;

        vec3 bestNormal =
            centerNormal;


        // --------------------------------------------------------
        // Search the full-resolution footprint
        // --------------------------------------------------------

        for(int y = 0; y < 4; y++)
        {
            for(int x = 0; x < 4; x++)
            {
                // Stop at actual search size.
                if(x >= searchSize ||
                   y >= searchSize)
                {
                    continue;
                }


                // ------------------------------------------------
                // Position inside low-res footprint.
                //
                // Half:
                //
                // 0 1
                // 2 3
                //
                // Quarter:
                //
                // 0 1 2 3
                // ...
                // ------------------------------------------------

                vec2 sampleOffset =
                    vec2(
                        float(x),
                        float(y)
                    )
                    -
                    (
                        float(searchSize) - 1.0
                    ) * 0.5;


                // ------------------------------------------------
                // Full-res sample UV
                // ------------------------------------------------

                vec2 sampleUV =
                    centerUV +
                    sampleOffset *
                    screenTexelSize;


                sampleUV =
                    clamp(
                        sampleUV,
                        screenTexelSize * 0.5,
                        vec2(1.0) -
                        screenTexelSize * 0.5
                    );


                // ------------------------------------------------
                // Sample geometry
                // ------------------------------------------------

                float sampleDepth =
                    Linear01Depth(
                        texture(
                            gDepth,
                            sampleUV
                        ).r
                    );

                vec3 sampleNormal =
                    GetNormal(sampleUV);


                // ------------------------------------------------
                // Depth similarity
                // ------------------------------------------------

                float depthDifference =
                    abs(
                        sampleDepth -
                        centerDepth
                    );

                float relativeDepth =
                    depthDifference /
                    max(
                        centerDepth,
                        1e-4
                    );

                float depthScore =
                    relativeDepth *
                    depthSigma;


                // ------------------------------------------------
                // Normal similarity
                // ------------------------------------------------

                float normalDot =
                    max(
                        dot(
                            centerNormal,
                            sampleNormal
                        ),
                        0.0
                    );

                float normalDifference =
                    1.0 -
                    normalDot;

                float normalScore =
                    normalDifference *
                    normalSigma;


                // ------------------------------------------------
                // Spatial preference
                // ------------------------------------------------

                float spatialDistance =
                    dot(
                        sampleOffset,
                        sampleOffset
                    );

                float spatialScore =
                    spatialDistance *
                    spatialSigma;


                // ------------------------------------------------
                // Final surface score
                // ------------------------------------------------
                //
                // Lower = better.
                //
                // We want:
                //
                // 1. Same depth
                // 2. Same normal
                // 3. Close to center
                // ------------------------------------------------

                float score =
                    depthScore +
                    normalScore +
                    spatialScore;


                // ------------------------------------------------
                // Select best representative surface
                // ------------------------------------------------

                if(score < bestScore)
                {
                    bestScore =
                        score;

                    bestDepth =
                        sampleDepth;

                    bestNormal =
                        sampleNormal;
                }
            }
        }


        // --------------------------------------------------------
        // Output
        //
        // RGB = representative normal
        // A   = representative linear depth
        // --------------------------------------------------------

        fragColor =
            vec4(
                bestNormal,
                bestDepth
            );
    }
    #endif

    #if defined(UpSample)
    // ============================================================
    // Spatial weight
    // ============================================================

    float SpatialWeight(vec2 offset)
    {
        float d2 =
            dot(
                offset,
                offset
            );

        return exp(
            -d2 *
            spatialSigma
        );
    }


    // ============================================================
    // Depth weight
    // ============================================================

    float DepthWeight(
        float centerDepth,
        float sampleDepth
    )
    {
        float difference =
            abs(
                sampleDepth -
                centerDepth
            );

        float relativeDifference =
            difference /
            max(
                centerDepth,
                1e-4
            );

        float x =
            relativeDifference *
            depthSigma;

        return exp(
            -x * x
        );
    }


    // ============================================================
    // Normal weight
    // ============================================================

    float NormalWeight(
        vec3 centerNormal,
        vec3 sampleNormal
    )
    {
        float Ndot =
            max(
                dot(
                    centerNormal,
                    sampleNormal
                ),
                0.0
            );

        return pow(
            Ndot,
            normalPower
        );
    }


    // ============================================================
    // Bilateral Upsample
    // ============================================================

    vec4 BilateralUpsample(vec2 uv)
    {
        // --------------------------------------------------------
        // Full-resolution guide
        // --------------------------------------------------------

        float centerDepth =
            Linear01Depth(
                texture(
                    gDepth,
                    uv
                ).r
            );

        vec3 centerNormal =
            GetNormal(uv);


        // --------------------------------------------------------
        // Convert full-res UV into low-res texel space
        // --------------------------------------------------------

        vec2 lowPosition =
            uv * giSize -
            0.5;

        vec2 basePosition =
            floor(
                lowPosition
            );

        vec2 fracPosition =
            fract(
                lowPosition
            );


        // --------------------------------------------------------
        // Accumulation
        // --------------------------------------------------------

        vec3 gi =
            vec3(0.0);

        float ao =
            0.0;

        float weightSum =
            0.0;


        // --------------------------------------------------------
        // 5x5 low-resolution reconstruction
        // --------------------------------------------------------

        const int RADIUS = 2;

        for(int y = -RADIUS; y <= RADIUS; y++)
        {
            for(int x = -RADIUS; x <= RADIUS; x++)
            {
                vec2 offset =
                    vec2(
                        float(x),
                        float(y)
                    );


                // ------------------------------------------------
                // Low-res texel center
                // ------------------------------------------------

                vec2 samplePosition =
                    basePosition +
                    offset +
                    vec2(0.5);


                vec2 sampleUV =
                    samplePosition /
                    giSize;


                sampleUV =
                    clamp(
                        sampleUV,
                        giTexelSize * 0.5,
                        vec2(1.0) -
                        giTexelSize * 0.5
                    );


                // ------------------------------------------------
                // GI/AO
                // ------------------------------------------------

                vec4 giSample =
                    texture(
                        giLow,
                        sampleUV
                    );


                // ------------------------------------------------
                // Surface-aware low-res geometry
                // ------------------------------------------------

                vec4 surface =
                    texture(
                        giSurface,
                        sampleUV
                    );


                vec3 sampleNormal =
                    normalize(
                        surface.rgb
                    );

                float sampleDepth =
                    surface.a;


                // ------------------------------------------------
                // Spatial
                //
                // Important:
                //
                // fracPosition tells us where the current
                // full-resolution pixel is inside the low-res
                // reconstruction grid.
                // ------------------------------------------------

                vec2 spatialOffset =
                    offset -
                    fracPosition;

                float spatialWeight =
                    SpatialWeight(
                        spatialOffset
                    );


                // ------------------------------------------------
                // Depth
                // ------------------------------------------------

                float depthWeight =
                    DepthWeight(
                        centerDepth,
                        sampleDepth
                    );


                // ------------------------------------------------
                // Normal
                // ------------------------------------------------

                float normalWeight =
                    NormalWeight(
                        centerNormal,
                        sampleNormal
                    );


                // ------------------------------------------------
                // Final bilateral weight
                // ------------------------------------------------

                float weight =
                    spatialWeight *
                    depthWeight *
                    normalWeight;


                // ------------------------------------------------
                // Accumulate
                // ------------------------------------------------

                gi +=
                    giSample.rgb *
                    weight;

                ao +=
                    giSample.a *
                    weight;

                weightSum +=
                    weight;
            }
        }


        // --------------------------------------------------------
        // Normalize
        // --------------------------------------------------------

        float invWeight =
            1.0 /
            max(
                weightSum,
                1e-5
            );

        gi *=
            invWeight;

        ao *=
            invWeight;


        // --------------------------------------------------------
        // Output
        // --------------------------------------------------------

        return vec4(
            gi,
            ao
        );
    }


    // ============================================================
    // Main
    // ============================================================

    void main()
    {
        fragColor =
            BilateralUpsample(
                texCoord
            );
    }
    #endif

    #if defined(UpSample2)
    // ============================================================
    // Spatial Weight
    // ============================================================
    //
    // offset:
    //     sample position in LOW-RES texel space
    //
    // fracPos:
    //     location of the current FULL-RES pixel inside the
    //     low-resolution texel.
    //
    // This makes the reconstruction work correctly for:
    //
    //     2x
    //     3x
    //     4x
    //     ...
    //
    // instead of assuming half resolution.
    //
    // ============================================================

    float SpatialWeight(
        vec2 offset,
        vec2 fracPos
    )
    {
        vec2 d = offset - fracPos;

        float d2 = dot(d, d);

        return exp(
            -d2 * spatialSigma
        );
    }


    // ============================================================
    // Depth Weight
    // ============================================================
    //
    // Uses relative depth difference.
    //
    // This behaves better across large depth ranges than using
    // a fixed absolute depth threshold.
    //
    // ============================================================

    float DepthWeight(
        float centerDepth,
        float sampleDepth
    )
    {
        float difference =
            abs(sampleDepth - centerDepth);

        float relativeDifference =
            difference /
            max(centerDepth, 1e-4);

        float x =
            relativeDifference *
            depthSigma;

        return exp(
            -x * x
        );
    }


    // ============================================================
    // Normal Weight
    // ============================================================

    float NormalWeight(
        vec3 centerNormal,
        vec3 sampleNormal
    )
    {
        float normalDot =
            max(
                dot(centerNormal, sampleNormal),
                0.0
            );

        return pow(
            normalDot,
            normalPower
        );
    }


    // ============================================================
    // Bilateral Upsample
    // ============================================================

    vec4 BilateralUpsample(vec2 uv)
    {
        // --------------------------------------------------------
        // Full-resolution guide
        // --------------------------------------------------------

        float centerDepth =
            Linear01Depth(
                texture(
                    gDepth,
                    uv
                ).r
            );

        vec3 centerNormal =
            GetNormal(uv);


        // --------------------------------------------------------
        // Convert full-resolution UV into low-resolution texel
        // coordinates.
        //
        // Example:
        //
        // Half:
        //     1920 -> 960
        //
        // Quarter:
        //     1920 -> 480
        //
        // --------------------------------------------------------

        vec2 lowPosition =
            uv * giSize - 0.5;


        // Integer low-resolution texel position.

        vec2 basePosition =
            floor(lowPosition);


        // Fractional position inside the low-resolution texel.

        vec2 fracPosition =
            fract(lowPosition);


        // --------------------------------------------------------
        // Accumulation
        // --------------------------------------------------------

        vec3 gi = vec3(0.0);

        float ao = 0.0;

        float weightSum = 0.0;


        // --------------------------------------------------------
        // 5x5 LOW-RES kernel
        //
        // Important:
        //
        // This is 5x5 low-resolution samples, NOT 5x5
        // full-resolution pixels.
        //
        // Therefore:
        //
        // Half resolution:
        //     each GI sample ~= 2x2 full pixels
        //
        // Quarter resolution:
        //     each GI sample ~= 4x4 full pixels
        //
        // The same kernel works for both.
        // --------------------------------------------------------

        const int RADIUS = 2;

        for(int y = -RADIUS; y <= RADIUS; y++)
        {
            for(int x = -RADIUS; x <= RADIUS; x++)
            {
                vec2 offset =
                    vec2(
                        float(x),
                        float(y)
                    );


                // ------------------------------------------------
                // Low-resolution texel coordinate
                //
                // +0.5 selects the center of the low-res texel.
                // ------------------------------------------------

                vec2 samplePosition =
                    basePosition +
                    offset +
                    vec2(0.5);


                // Convert low-res texel coordinate back to UV.

                vec2 sampleUV =
                    samplePosition /
                    giSize;


                // ------------------------------------------------
                // Clamp to valid texture area.
                // ------------------------------------------------

                sampleUV =
                    clamp(
                        sampleUV,

                        giTexelSize * 0.5,

                        vec2(1.0) -
                        giTexelSize * 0.5
                    );


                // ------------------------------------------------
                // Sample low-resolution GI/AO
                // ------------------------------------------------

                vec4 giSample =
                    texture(
                        giLow,
                        sampleUV
                    );


                // ------------------------------------------------
                // Sample FULL-resolution geometry guide.
                //
                // This is important:
                //
                // GI is low-res.
                //
                // Depth/normal are used as the high-resolution
                // guide for edge-aware reconstruction.
                // ------------------------------------------------

                float sampleDepth =
                    Linear01Depth(
                        texture(
                            gDepth,
                            sampleUV
                        ).r
                    );


                vec3 sampleNormal =
                    GetNormal(sampleUV);


                // ------------------------------------------------
                // Spatial
                // ------------------------------------------------

                float spatialWeight =
                    SpatialWeight(
                        offset,
                        fracPosition
                    );


                // ------------------------------------------------
                // Depth
                // ------------------------------------------------

                float depthWeight =
                    DepthWeight(
                        centerDepth,
                        sampleDepth
                    );


                // ------------------------------------------------
                // Normal
                // ------------------------------------------------

                float normalWeight =
                    NormalWeight(
                        centerNormal,
                        sampleNormal
                    );


                // ------------------------------------------------
                // Final bilateral weight
                // ------------------------------------------------

                float weight =
                    spatialWeight *
                    depthWeight *
                    normalWeight;


                // ------------------------------------------------
                // Accumulate
                // ------------------------------------------------

                gi +=
                    giSample.rgb *
                    weight;

                ao +=
                    giSample.a *
                    weight;

                weightSum += weight;
            }
        }


        // --------------------------------------------------------
        // Normalize
        // --------------------------------------------------------

        float invWeight =
            1.0 /
            max(
                weightSum,
                1e-5
            );


        gi *= invWeight;

        ao *= invWeight;


        // --------------------------------------------------------
        // Result
        // --------------------------------------------------------

        return vec4(
            gi,
            ao
        );
    }


    // ============================================================
    // Main
    // ============================================================

    void main()
    {
        fragColor =
            BilateralUpsample(texCoord);
    }
    #endif

#endif
