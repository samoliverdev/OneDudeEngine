#pragma BeginPassDef
    Name BlurPass
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Common.glsl

Texture2D(0, 5, mainTex, mainTexSampler) // SSGI output (color * visibility + lighting)
Texture2D(0, 7,  gNormal,        gNormalSampler)
Texture2D(0, 12, gDepth,         gDepthSampler)

BeginUniform(0, 0, Main)
    Uniform vec2 screenSize; // Downsampled resolution (e.g., 960x540)
    Uniform float blurRadius; // Blur strength (e.g., 1.0-3.0 pixels)
    Uniform float depthThreshold; // Depth difference for edge preservation (e.g., 0.1-1.0)
    Uniform vec2 giTexelSize;
EndUniform()

#if defined(VERTEX) && defined(BlurPass)
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

#if defined(FRAGMENT) && defined(BlurPass)
    in vec3 pos;
    in vec2 texCoord;
    out vec4 fragColor;

    #include Engine/ShaderLibrary/Vertex.glsl

    // 5x5 Gaussian weights (σ ≈ 1.0, normalized)
    const float gaussian[5] = float[](0.06136, 0.24477, 0.38774, 0.24477, 0.06136);
    const float offsets[5] = float[](0.06136, 0.24477, 0.38774, 0.24477, 0.06136);

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

    //------------------------------------------------------------------------------
    //  Copyright (c) 2018-2024 Michele Morrone
    //  All rights reserved.
    //
    //  https://michelemorrone.eu - https://brutpitt.com
    //
    //  X: https://x.com/BrutPitt - GitHub: https://github.com/BrutPitt
    //
    //  direct mail: brutpitt(at)gmail.com - me(at)michelemorrone.eu
    //
    //  This software is distributed under the terms of the BSD 2-Clause license
    //------------------------------------------------------------------------------
    #define INV_SQRT_OF_2PI 0.39894228040143267793994605993439  // 1.0/SQRT_OF_2PI
    #define INV_PI 0.31830988618379067153776752674503

    //  smartDeNoise - parameters
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //
    //  sampler2D tex     - sampler image / texture
    //  vec2 uv           - actual fragment coord
    //  float sigma  >  0 - sigma Standard Deviation
    //  float kSigma >= 0 - sigma coefficient 
    //      kSigma * sigma  -->  radius of the circular kernel
    //  float threshold   - edge sharpening threshold 

    vec4 smartDeNoise(sampler2D tex, vec2 uv, float sigma, float kSigma, float threshold){
        float radius = round(kSigma*sigma);
        float radQ = radius * radius;

        float invSigmaQx2 = .5 / (sigma * sigma);      // 1.0 / (sigma^2 * 2.0)
        float invSigmaQx2PI = INV_PI * invSigmaQx2;    // 1/(2 * PI * sigma^2)

        float invThresholdSqx2 = .5 / (threshold * threshold);     // 1.0 / (sigma^2 * 2.0)
        float invThresholdSqrt2PI = INV_SQRT_OF_2PI / threshold;   // 1.0 / (sqrt(2*PI) * sigma^2)

        vec4 centrPx = texture(tex,uv); 

        float zBuff = 0.0;
        vec4 aBuff = vec4(0.0);
        vec2 size = vec2(textureSize(tex, 0));

        vec2 d;
        for(d.x=-radius; d.x <= radius; d.x++){
            float pt = sqrt(radQ-d.x*d.x);       // pt = yRadius: have circular trend
            for(d.y=-pt; d.y <= pt; d.y++){
                float blurFactor = exp( -dot(d , d) * invSigmaQx2 ) * invSigmaQx2PI;

                vec4 walkPx =  texture(tex,uv+d/size);
                vec4 dC = walkPx-centrPx;
                float deltaFactor = exp( -dot(dC, dC) * invThresholdSqx2) * invThresholdSqrt2PI * blurFactor;

                zBuff += deltaFactor;
                aBuff += deltaFactor*walkPx;
            }
        }
        return aBuff/zBuff;
    }

    void main(){
        //vec4 color = smartDeNoise(mainTex, texCoord, 5.0, 2.0, 0.250);
        //vec4 color = texture(mainTex, texCoord);
        //fragColor = vec4(color.rgb, 1);
        //return;

        ///*
        vec2 uv = texCoord;

        vec3 centerGI = texture(mainTex, uv).rgb;
        float centerDepth = texture(gDepth, uv).r;
        vec3 centerNormal = GetViewNormal(uv);

        vec3 sum = centerGI;
        float wsum = 1.0;

        for(int i = 0; i < 4; ++i){
            vec2 off = offsets[i] * giTexelSize;
            vec2 suv = uv + off;

            vec3 gi = texture(mainTex, suv).rgb;
            float d = abs(texture(gDepth, suv).r - centerDepth);
            vec3 n = GetViewNormal(suv);

            float wn = pow(max(dot(centerNormal, n), 0.0), 8.0);
            float wd = exp(-d * 50.0);

            float w = wn * wd;
            sum += gi * w;
            wsum += w;
        }

        vec3 blurred = sum / wsum;
        fragColor = vec4(blurred, 1.0);
        //*/

        /*
        vec3 centerColor = texture(mainTex, texCoord).rgb;
        vec3 position = GetViewPos(texCoord); //texture(gPosition, texCoord).rgb; position = (view * vec4(position, 1)).xyz;
        float centerDepth = position.z; 
  
        vec3 blurredColor = vec3(0.0);
        float weightSum = 0.0;
        vec2 pixelSize = 1.0 / screenSize; // Pixel size in UV space

        // 5x5 bilateral blur
        for(int i = -2; i <= 2; i++){
            for(int j = -2; j <= 2; j++){
                vec2 offset = vec2(i, j) * blurRadius * pixelSize;
                vec2 sampleUV = texCoord + offset;

                vec3 sampleColor = texture(mainTex, sampleUV).rgb;
                vec3 _pos = GetViewPos(sampleUV); //texture(gPosition, sampleUV).rgb; _pos = (view * vec4(position, 1)).xyz;
                float sampleDepth = _pos.z;

                // Depth weight (bilateral)
                float depthDiff = abs(sampleDepth - centerDepth);
                float depthWeight = exp(-depthDiff * depthDiff / (2.0 * depthThreshold * depthThreshold));

                // Gaussian weight
                float spatialWeight = gaussian[abs(i)] * gaussian[abs(j)];

                // Combined weight
                float weight = spatialWeight * depthWeight;

                blurredColor += sampleColor * weight;
                weightSum += weight;
            }
        }

        if(weightSum > 0.001){
            blurredColor /= weightSum;
        } else {
            blurredColor = centerColor; // Fallback if no valid samples
        }

        fragColor = vec4(blurredColor, 1.0);
        */
    }
#endif