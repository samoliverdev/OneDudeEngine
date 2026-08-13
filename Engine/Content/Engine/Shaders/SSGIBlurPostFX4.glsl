#pragma BeginProperties 
    Texture2D mainTex White 
#pragma EndProperties 
 
#pragma BeginPassDef 
    Name AtrousPass 
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef 
 
#include Engine/ShaderLibrary/Base.glsl 
#include Engine/ShaderLibrary/Core.glsl 
#include Engine/ShaderLibrary/Common.glsl
 
BeginUniform(0, 0, Main) 
    Uniform vec4 color; 
    Uniform vec2 giSize; 
    Uniform vec2 screenSize;
    // 1, 2, 4, 8
    Uniform float atrousStep;
EndUniform() 

Texture2D(0, 1, mainTex, mainSampler) 
Texture2D(0, 1, gAlbedoSpec, gAlbedoSpecSampler) 

Texture2D(0, 1, gNormal, gNormalSampler)
Texture2D(0, 2, gDepth, gDepthSampler)

Texture2D(0, 2, giAO, giAOSampler) 
 
#if defined(VERTEX) 
    In(0) vec3 vPos; 
    In(1) vec2 vTexCoord; 

    Out(0) vec3 pos; 
    Out(1) vec2 texCoord; 
 
    void main(){ 
        pos = vPos; 
        texCoord = vTexCoord;  
        OutPosition = vec4(pos, 1.0); 
    } 
#endif 
 
#if defined(FRAGMENT) 
    In(0) vec3 pos; 
    In(1) vec2 texCoord; 

    Out(0) vec4 fragColor;

    vec3 GetNormal(vec2 uv){
        //return normalize(texture(gNormal, uv).xyz * 2.0 - 1.0);
        return normalize(unpack_normal_octahedron(texture(gNormal, uv).rg));
    }

    float Luminance(vec3 c){
        return dot(c, vec3(0.2126, 0.7152, 0.0722));
    }

    float AtrousKernelWeight(int x, int y){
        const float kernel[5] = float[](
            1.0,
            4.0,
            6.0,
            4.0,
            1.0
        );
        return kernel[x + 2] * kernel[y + 2] / 256.0;
    }

    float NormalWeight(vec3 centerNormal, vec3 sampleNormal){
        float ndot = max(dot(centerNormal, sampleNormal), 0.0);

        // Higher = stronger edge rejection.
        // 8  = soft
        // 16 = moderate
        // 32 = strong
        // 64 = very strong
        return pow(ndot, 32.0);
    }

    float DepthWeight(float centerDepth, float sampleDepth){
        float difference = abs(centerDepth - sampleDepth);

        // For raw depth-buffer values.
        // This is intentionally relatively strong.
        return exp(-difference * 100.0);
    }

    float GILuminanceWeight(vec3 centerGI, vec3 sampleGI){
        float centerLuma = Luminance(centerGI);
        float sampleLuma = Luminance(sampleGI);
        float difference = abs(centerLuma - sampleLuma);

        // Keep this weaker than depth/normal.
        return exp(-difference * 2.0);
    }

    vec4 AtrousFilter(vec2 uv, vec2 texelSize, float stepSize){
        vec4 centerGI = SampleTexture2D(mainTex, giAOSampler, uv);
        vec3 centerNormal = GetNormal(uv).rgb;
        centerNormal = normalize(centerNormal);

        float centerDepth = SampleTexture2D(gDepth, gDepthSampler, uv).r;

        vec3 resultGI = vec3(0.0);
        float resultAO = 0.0;
        float totalWeight = 0.0;

        // 5x5 À-TROUS KERNEL
        for(int y = -2; y <= 2; ++y){
            for(int x = -2; x <= 2; ++x){
                // Increasing step:
                // pass 0 = 1
                // pass 1 = 2
                // pass 2 = 4
                // pass 3 = 8
                vec2 offset = vec2(x, y) * texelSize * stepSize;
                vec2 sampleUV = uv + offset;

                // GI
                vec4 sampleGI = SampleTexture2D(mainTex, giAOSampler, sampleUV);

                // NORMAL
                vec3 sampleNormal = GetNormal(sampleUV).rgb;
                sampleNormal = normalize(sampleNormal);

                // DEPTH
                float sampleDepth = SampleTexture2D(gDepth, gDepthSampler, sampleUV).r;

                // BASE KERNEL
                float weight = AtrousKernelWeight(x, y);

                // NORMAL EDGE STOPPING
                weight *= NormalWeight(centerNormal, sampleNormal);

                // DEPTH EDGE STOPPING
                weight *= DepthWeight(centerDepth, sampleDepth);

                // GI LUMINANCE
                weight *= GILuminanceWeight(centerGI.rgb, sampleGI.rgb);

                // ACCUMULATE
                resultGI += sampleGI.rgb * weight;
                resultAO += sampleGI.a * weight;
                totalWeight += weight;
            }
        }

        totalWeight = max(totalWeight, 0.00001);
        return vec4(
            resultGI / totalWeight,
            resultAO / totalWeight
        );
    }

    void main(){
        // Atrous is running at GI resolution.
        vec2 giTexelSize = 1.0 / giSize;
        fragColor = AtrousFilter(texCoord, giTexelSize, atrousStep);
    }
#endif