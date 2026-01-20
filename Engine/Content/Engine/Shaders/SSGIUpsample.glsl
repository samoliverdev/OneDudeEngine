BeginProperties
    Texture2D mainTex White
EndProperties

BeginPass
    #pragma Name MainPass

    #include Engine/ShaderLibrary/Base.glsl
    #include Engine/ShaderLibrary/Core.glsl

    BeginUniform(0, 0, Main)
        Uniform vec4 color;
        Uniform vec2 giTexelSize;
        Uniform vec2 screenSize;
        Uniform float nearPlane;
        Uniform float farPlane;
    EndUniform()
    Texture2D(0, 1, mainTex, mainSampler)
    Texture2D(0, 12, giLow, giLowSampler)
    Texture2D(0, 12, gDepth, gDepthSampler)
    Texture2D(0, 7, gNormal, gNormalSampler)

    BeginVertex
    In(0) vec3 vPos;
    In(1) vec2 vTexCoord;
    Out(0) vec3 pos;
    Out(1) vec2 texCoord;

    void main() {
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

    float LinearizeDepth(float d){
        return (nearPlane * farPlane) / (farPlane - d * (farPlane - nearPlane));
    }

    void main(){
        vec2 vUV = texCoord;

        float centerDepth = LinearizeDepth(texture(gDepth, vUV).r);
        vec3 centerNormal = normalize(texture(gNormal, vUV).xyz * 2.0 - 1.0);

        vec3 gi = vec3(0.0);
        float ao = 0.0;
        float weightSum = 0.0;

        for(int y = -1; y <= 1; y++){
            for(int x = -1; x <= 1; x++){
                vec2 offset = vec2(x, y) * giTexelSize;
                vec2 uvLow = vUV + offset;

                vec4 giSample = texture(giLow, uvLow);

                float sampleDepth = LinearizeDepth(texture(gDepth, vUV + offset).r);
                vec3 sampleNormal = normalize(texture(gNormal, vUV + offset).xyz * 2.0 - 1.0);

                float depthWeight  = exp(-abs(sampleDepth - centerDepth) * 50.0);
                float normalWeight = max(dot(centerNormal, sampleNormal), 0.0);
                float w = depthWeight * normalWeight;

                gi += giSample.rgb * w;
                ao += giSample.a   * w;
                weightSum += w;
            }
        }

        gi /= max(weightSum, 1e-4);
        ao /= max(weightSum, 1e-4);

        fragColor = vec4(gi, ao);
    }
    EndFrag
EndPass
