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
        Uniform vec3 View_WorldCameraOrigin;
        Uniform vec2 g_resolution;
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

    float LinearEyeDepth(float depth){
        float z = depth * 2.0 - 1.0;
        return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
    }

    float Linear01Depth(float depth){
        float z = depth * 2.0 - 1.0; // Back to NDC
        float linearDepth = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
        return linearDepth / farPlane;
    }

    #define SSS_MAX_STEPS 64 //32
    #define SSS_RAY_LENGTH 1.0 //1.0 //1.0
    #define SSS_THICKNESS 0.1 //0.05
    #define SSS_BIAS 0.035 //0 //1e-04
    #define SSS_FADE_DISTANCE 500.0

    vec3 ReconstructViewPosition(vec2 uv, float depth){
        vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
        vec4 viewPos = invProjection * clip;
        return viewPos.xyz / viewPos.w;
    }

    vec2 ProjectUV(vec3 viewPos){
        vec4 clip = projection * vec4(viewPos, 1.0);
        vec3 ndc = clip.xyz / clip.w;
        return ndc.xy * 0.5 + 0.5;
    }

    float InterleavedGradientNoise(vec2 pixCoord, int frameCount){
        const vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
        const vec2 frameMagicScale = vec2(2.083f, 4.867f);
        pixCoord += frameCount * frameMagicScale;
        return fract(magic.z * fract(dot(pixCoord, magic.xy)));
    }

    float ScreenSpaceShadow(
        vec3 worldPosition,
        vec3 worldNormal,
        vec3 worldLightDirection,
        float random
    ){  
        //INFO: Dont work
        // 1. Reject backfaces
        //float NdotL = dot(worldNormal, worldLightDirection);
        //if(NdotL <= 0.0) return 1.0;

        // world -> view
        vec3 rayPos = (view * vec4(worldPosition, 1.0)).xyz;

        // light direction world -> view
        vec3 rayDir = (view * vec4(-worldLightDirection, 0.0)).xyz;

        rayDir = normalize(rayDir);

        float stepLength = SSS_RAY_LENGTH / float(SSS_MAX_STEPS);
        vec3 rayStep = rayDir * stepLength;// * random;

        // jitter
        float offset = random;// * 2.0 - 1.0;
        rayPos += rayStep * offset;

        float shadow = 0.0;

        for(int i = 0; i < SSS_MAX_STEPS; i++){
            rayPos += rayStep;// * random;

            vec2 uv = ProjectUV(rayPos);

            // outside screen
            if(any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))){
                //continue;
                break;
            }

            float sceneDepth = texture(gDepth, uv).r;
            float sceneViewDepth = LinearEyeDepth(sceneDepth);

            // OpenGL view space looks down -Z
            float rayDepth = -rayPos.z;
            float depthDelta = rayDepth - sceneViewDepth;

            if(depthDelta > SSS_BIAS && depthDelta < SSS_THICKNESS){
                shadow = 1.0;
                break;
            }

            // thickness grows with distance
            /*float thickness = SSS_THICKNESS * (1.0 + rayDepth * 0.1);
            if(depthDelta > SSS_BIAS && depthDelta < thickness){
                shadow = 1.0;
                break;
            }*/
        }

         // distance fade
        float rayDepth = -(view * vec4(worldPosition,1)).z;
        float fade = clamp(rayDepth / SSS_FADE_DISTANCE, 0.0, 1.0);
        shadow *= (1.0 - fade);

        return 1.0 - shadow;
    }

    void main(){
        float depth = SampleDepth(texCoord);
        float s = ScreenSpaceShadow(
            GetPos(texCoord, depth),
            SampleNormal(texCoord),
            -lightDirection,
            InterleavedGradientNoise(g_resolution * texCoord, 0)
        );

        fragColor = vec4(s);
        //fragColor = SampleTexture2D(mainTex, mainSampler, texCoord); //texture(mainTex, texCoord);
    }
    EndFrag
EndPass
