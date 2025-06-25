#pragma BeginPassDef
    Name BlurPass
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

Texture2D(0, 5, mainTex, mainTexSampler) // SSGI output (color * visibility + lighting)
Texture2D(0, 6, gPosition, gPositionSampler) // World-space position or depth

BeginUniform(0, 0, Main)
    Uniform vec2 screenSize; // Downsampled resolution (e.g., 960x540)
    Uniform float blurRadius; // Blur strength (e.g., 1.0-3.0 pixels)
    Uniform float depthThreshold; // Depth difference for edge preservation (e.g., 0.1-1.0)
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

    void main() {
        vec3 centerColor = texture(mainTex, texCoord).rgb;
        vec3 position = texture(gPosition, texCoord).rgb; position = (view * vec4(position, 1)).xyz;
        float centerDepth = position.z; 
        /*if(centerDepth < 0.001) {
            fragColor = vec4(centerColor, 1.0); // Skip blur for invalid depth
            return;
        }*/

        vec3 blurredColor = vec3(0.0);
        float weightSum = 0.0;
        vec2 pixelSize = 1.0 / screenSize; // Pixel size in UV space

        // 5x5 bilateral blur
        for (int i = -2; i <= 2; i++) {
            for (int j = -2; j <= 2; j++) {
                vec2 offset = vec2(i, j) * blurRadius * pixelSize;
                vec2 sampleUV = texCoord + offset;

                // Skip out-of-bounds samples
                /*if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                    continue;
                }*/

                vec3 sampleColor = texture(mainTex, sampleUV).rgb;
                vec3 _pos = texture(gPosition, sampleUV).rgb; _pos = (view * vec4(position, 1)).xyz;
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

        // Normalize
        if (weightSum > 0.001) {
            blurredColor /= weightSum;
        } else {
            blurredColor = centerColor; // Fallback if no valid samples
        }

        fragColor = vec4(blurredColor, 1.0);
    }
#endif