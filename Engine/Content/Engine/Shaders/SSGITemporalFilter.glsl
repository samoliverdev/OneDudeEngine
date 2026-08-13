#pragma BeginPassDef
    Name Pass1
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Core.glsl

BeginUniform(0, 0, Main)
    Uniform mat4 lastProj;
    Uniform mat4 lastView;
    Uniform mat4 lastInvProj;
    Uniform mat4 lastInvView;
EndUniform()

Texture2D(0, 3, gDepth, gDepthSampler)
Texture2D(0, 2, giAO, giAOSampler)
Texture2D(0, 3, gDepthHistory, gDepthSampler)
Texture2D(0, 2, giAOHistory, giAOSampler)

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
    #include Engine/ShaderLibrary/Common.glsl

    float GetDepth(vec2 uv){
        return texture(gDepth, uv).r;
    } 

    float GetPreviousDepth(vec2 uv){
        return texture(gDepthHistory, uv).r;
    } 

    #if defined(Pass1)
    void main(){
        vec2 uv = texCoord;

        // Current pixel -> current world position
        float currentDepth = GetDepth(uv);

        vec3 worldPosition = reconstructWorldPos(
            uv,
            currentDepth,
            invProjection,
            invView
        );

        // Current world position -> previous frame
        vec4 previousClip = lastProj * lastView * vec4(worldPosition, 1.0);

        if(previousClip.w <= 0.0){
            fragColor = texture(giAO, uv);
            return;
        }

        previousClip /= previousClip.w;

        vec2 previousUV = previousClip.xy * 0.5 + 0.5;

        // Check if previous UV is inside screen
        if (previousUV.x < 0.0 ||
            previousUV.x > 1.0 ||
            previousUV.y < 0.0 ||
            previousUV.y > 1.0)
        {
            fragColor = texture(giAO, uv);
            return;
        }

        // Sample current / history
        vec4 current = texture(giAO, uv);
        vec4 history = texture(giAOHistory, previousUV);


        vec2 texelSize = 1.0 / vec2(textureSize(giAO, 0));
        vec4 minAO = current;
        vec4 maxAO = current;
        for(int y = -1; y <= 1; ++y){
            for(int x = -1; x <= 1; ++x){
                vec2 p = uv + vec2(x, y) * texelSize;

                vec4 ao = texture(giAO, p);

                minAO = min(minAO, ao);
                maxAO = max(maxAO, ao);
            }
        }

        history = clamp(history, minAO, maxAO);

        // History depth
        float previousDepth = GetPreviousDepth(previousUV);

        // Depth rejection
        float depthDifference = abs(currentDepth - previousDepth);

        float valid = 1.0;

        if(depthDifference > 0.002) valid = 0.0;

        // Temporal blend
        float historyWeight = 0.85 * valid;
        fragColor = mix(current, history, historyWeight);
    }
    #endif
#endif
