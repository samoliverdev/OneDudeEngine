BeginProperties
    Texture2D mainTex White
EndProperties

BeginPass
    #pragma Name MainPass

    #include Engine/ShaderLibrary/Base.glsl
    #include Engine/ShaderLibrary/Core.glsl
    #include Engine/ShaderLibrary/Common.glsl

    BeginUniform(0, 0, Main)
        Uniform mat4 View_WorldToClip;
        Uniform vec4 View_InvDeviceZToWorldZTransform;
        Uniform vec3 lightDirection;
        Uniform float nearPlane;
        Uniform float farPlane;
    EndUniform()
    Texture2D(0, 7,  gNormal,        gNormalSampler)
    Texture2D(0, 12, gDepth, gDepthSampler)

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

    #include Engine/ShaderLibrary/Vertex.glsl

    float SampleDepth(vec2 uv){
        return texture(gDepth, uv).r;
    } 

    vec3 GetPos(vec2 uv, float depth){
        vec3 wp = reconstructWorldPos(
            uv,
            depth,
            invProjection,
            invView
        );
        return wp; //(view * vec4(wp, 1.0)).xyz;
    }

    vec3 SampleNormal(vec2 uv){
        vec3 n = unpack_normal_octahedron(texture(gNormal, uv).rg);
        return normalize(n); //mat3(view) * n);
    }

    //how many marches we do against the scene depth buffer
    #define CONTACT_SHADOWS_SAMPLES 64

    //how far out do the rays go
    #define CONTACT_SHADOWS_RAY_LENGTH 100.0

    //assumed thickness of the depth
    #define CONTACT_SHADOWS_THICKNESS 0.35

    //offset to our starting position so we avoid self-occlusion artifacts
    #define CONTACT_SHADOWS_BIAS 1e-04

    //offset to our starting position using the surface normal so we avoid self-occlusion artifacts
    #define CONTACT_SHADOWS_NORMAL_BIAS 1e-04

    float InterleavedGradientNoise(vec2 pixCoord, int frameCount){
        const vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
        const vec2 frameMagicScale = vec2(2.083f, 4.867f);
        pixCoord += frameCount * frameMagicScale;
        return fract(magic.z * fract(dot(pixCoord, magic.xy)));
    }

    float LinearEyeDepth(float depth){
        float z = depth * 2.0 - 1.0;
        return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
    }

    float ContactShadowClipSpace(
        vec3 worldPosition,
        vec3 worldNormal,
        vec3 worldLightDirection,
        float randomValue
    ){
        // 1.0 / samples
        float invSamples = 1.0 / float(CONTACT_SHADOWS_SAMPLES);

        vec3 rayOrigin = worldPosition;

        vec4 clipStart = View_WorldToClip * vec4(rayOrigin, 1.0);
        vec4 clipEnd = View_WorldToClip * vec4(rayOrigin + worldLightDirection * CONTACT_SHADOWS_RAY_LENGTH, 1.0);

        vec3 ndcStart = clipStart.xyz / clipStart.w;
        vec3 ndcEnd   = clipEnd.xyz / clipEnd.w;

        float rayLinearStart = LinearEyeDepth(ndcStart.z);
        float rayLinearEnd   = LinearEyeDepth(ndcEnd.z);
        //float rayLinearStart = LinearEyeDepth(ndcStart.z * 0.5 + 0.5);
        //float rayLinearEnd = LinearEyeDepth(ndcEnd.z * 0.5 + 0.5);

        float rayLinearDepth = mix(rayLinearStart, rayLinearEnd, randomValue * invSamples);

        float rayLinearStep = (rayLinearEnd - rayLinearStart) * invSamples;
        //vec2 uvScale = vec2(0.5, -0.5);
        vec2 uvScale = vec2(0.5, 0.5);
        vec2 uvStep = (ndcEnd.xy - ndcStart.xy) * (invSamples * uvScale);
        vec2 uv = ndcStart.xy * uvScale + vec2(0.5) + uvStep * randomValue;
        for(int i = 0; i < CONTACT_SHADOWS_SAMPLES; i++){
            if(any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))){
                break;
            }

            float depth = SampleDepth(uv); //textureLod(gDepth, uv, 0.0).r;

            // Equivalent of:
            // View_InvDeviceZToWorldZTransform.z /
            // (depth - View_InvDeviceZToWorldZTransform.w)

            float linearDepth = LinearEyeDepth(depth); //View_InvDeviceZToWorldZTransform.z / (depth - View_InvDeviceZToWorldZTransform.w);
            float penetration = rayLinearDepth - linearDepth;

            if(penetration > CONTACT_SHADOWS_BIAS && penetration < CONTACT_SHADOWS_THICKNESS){
                return 0.0;
            }

            rayLinearDepth += rayLinearStep;
            uv += uvStep;
        }

        return 1.0;
    }

    void main(){
        float depth = SampleDepth(texCoord);
        float s = ContactShadowClipSpace(
            GetPos(texCoord, depth),
            SampleNormal(texCoord),
            -lightDirection,
            InterleavedGradientNoise(texCoord, 0)
        );

        fragColor = vec4(s);
        //fragColor = SampleTexture2D(mainTex, mainSampler, texCoord); //texture(mainTex, texCoord);
    }
    EndFrag
EndPass
