#pragma BeginPassDef
    Name MainPass
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

//Source: https://cybereality.com/screen-space-indirect-lighting-with-visibility-bitmask-improvement-to-gtao-ssao-real-time-ambient-occlusion-algorithm-glsl-shader-implementation/

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Common.glsl

Texture2D(0, 5, mainTex, mainTexSampler)
Texture2D(0, 6, gPosition, gPositionSampler)
Texture2D(0, 7, gNormal, gNormalSampler)
Texture2D(0, 7, gAlbedoSpec, gAlbedoSpecSampler) 
Texture2D(0, 12, gDepth, gDepthSampler)
Texture2D(0, 11, noise, noiseSampler)
Texture2D(0, 12, lastIndirect, lastIndirectSampler)

BeginUniform(0, 0, Main)
    Uniform vec2 screenSize;
    Uniform float sampleCount;
    Uniform float sampleRadius;
    Uniform float sliceCount;
    Uniform float hitThickness;
    Uniform float giIntensity;
EndUniform()

#if defined(VERTEX) && defined(MainPass)
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

    #include Engine/ShaderLibrary/Vertex.glsl

    const float pi = 3.14159265359;
    const float twoPi = 2.0 * pi;
    const float halfPi = 0.5 * pi;

    float expFactor = 2.0;          // like Three.js
    float aoPower   = 1.5;          // AO shaping
    float giClamp   = 7.0;          // HDR energy clamp

    // GTAO fast acos approximation (from Activision)
    float fastAcos(float x) {
        float y = abs(x);
        float p = -0.156583 * y + halfPi;
        p *= sqrt(1.0 - y);
        return x >= 0.0 ? p : pi - p;
    }

    // Exponential sample distribution
    float expStep(float i, float count, float jitter, float expFactor) {
        float t = (i + jitter) / count;
        return pow(t, expFactor);
    }

    // https://blog.demofox.org/2022/01/01/interleaved-gradient-noise-a-different-kind-of-low-discrepancy-sequence/
    float randf(int x, int y) {
        return mod(52.9829189 * mod(0.06711056 * float(x) + 0.00583715 * float(y), 1.0), 1.0);
    }

    float randf(vec2 st) {
        return mod(52.9829189 * mod(0.06711056 * float(st.x) + 0.00583715 * float(st.y), 1.0), 1.0);
    }

    // Blue noise sampling function
    float getNoise(vec2 uv, float offset) {
        return texture(noise, uv * screenSize / 64.0 + vec2(offset, 0.0)).r - 0.5;
    }

    // https://graphics.stanford.edu/%7Eseander/bithacks.html
    /*uint bitCount(uint value) {
        value = value - ((value >> 1u) & 0x55555555u);
        value = (value & 0x33333333u) + ((value >> 2u) & 0x33333333u);
        return ((value + (value >> 4u) & 0xF0F0F0Fu) * 0x1010101u) >> 24u;
    }*/

    // https://cdrinmatane.github.io/posts/ssaovb-code/
    const uint sectorCount = 32u;
    uint updateSectors(float minHorizon, float maxHorizon, uint outBitfield) {
        uint startBit = uint(minHorizon * float(sectorCount));
        uint horizonAngle = uint(ceil((maxHorizon - minHorizon) * float(sectorCount)));
        uint angleBit = horizonAngle > 0u ? uint(0xFFFFFFFFu >> (sectorCount - horizonAngle)) : 0u;
        uint currentBitfield = angleBit << startBit;
        return outBitfield | currentBitfield;
    }

    //#define USE_BLUE_NOISE

    vec3 GetWorldPos(vec2 texCoord){
        return reconstructWorldPos(texCoord, texture(gDepth, texCoord).r, invProjection, invView);
    }

    vec3 GetWorldNormal(vec2 texCoord){
        return unpack_normal_octahedron(texture(gNormal, texCoord).rg);
    }

    void main(){
        uint indirect = 0u;
        uint occlusion = 0u;

        float visibility = 0.0;
        vec3 lighting = vec3(0.0);
        vec2 frontBackHorizon = vec2(0.0);
        vec2 aspect = screenSize.yx / screenSize.x;
        vec3 position = GetWorldPos(texCoord); position = (view * vec4(position, 1)).xyz;
        vec3 camera = normalize(-position);
        vec3 normal = normalize(GetWorldNormal(texCoord)); normal = normalize(mat3(view) * normal);
        float sliceRotation = twoPi / (sliceCount - 1.0);
        float sampleScale = (-sampleRadius * projection[0][0]) / position.z;
        float sampleOffset = 0.01;

        //float jitter = randf(int(gl_FragCoord.x), int(gl_FragCoord.y)) - 0.5;

        #ifdef USE_BLUE_NOISE
            float jitter = getNoise(texCoord, 0.0);
        #else
            float jitter = randf(int(gl_FragCoord.x), int(gl_FragCoord.y)) - 0.5;
        #endif

        for(float slice = 0.0; slice < sliceCount; slice += 1.0){
            float phi = sliceRotation * (slice + jitter);
            vec2 omega = vec2(cos(phi), sin(phi));

            // reset occlusion PER SLICE (important)
            occlusion = 0u;

            // two-sided sampling
            for(int side = 0; side < 2; side++){

                vec2 dir = (side == 0) ? omega : -omega;
                vec3 direction = vec3(dir, 0.0);

                vec3 orthoDirection = direction - dot(direction, camera) * camera;
                vec3 axis = cross(direction, camera);

                vec3 projNormal = normal - axis * dot(normal, axis);
                float projLen = max(length(projNormal), 1e-4);

                float signN = sign(dot(orthoDirection, projNormal));
                float cosN  = clamp(dot(projNormal, camera) / projLen, -1.0, 1.0);
                float n     = signN * fastAcos(cosN);

                for(float i = 0.0; i < sampleCount; i += 1.0){
                    float stepT = expStep(i, sampleCount, jitter, expFactor);
                    float step  = stepT * sampleRadius + sampleOffset;

                    vec2 sampleUV = texCoord - step * sampleScale * dir * aspect;
                    if(any(lessThan(sampleUV, vec2(0.0))) ||
                        any(greaterThan(sampleUV, vec2(1.0))))
                        break;

                    vec3 samplePos = GetWorldPos(sampleUV);
                    samplePos = (view * vec4(samplePos, 1)).xyz;

                    vec3 sampleDist = samplePos - position;
                    float len = max(length(sampleDist), 1e-4);
                    vec3 horizonDir = sampleDist / len;

                    // distance-scaled thickness
                    float thicknessScale = clamp(abs(position.z) * 0.02, 1.0, 5.0);
                    vec3 backPos = sampleDist - camera * hitThickness * thicknessScale;

                    frontBackHorizon.x = dot(horizonDir, camera);
                    frontBackHorizon.y = dot(normalize(backPos), camera);

                    frontBackHorizon = vec2(
                        fastAcos(frontBackHorizon.x),
                        fastAcos(frontBackHorizon.y)
                    );

                    frontBackHorizon =
                        clamp((frontBackHorizon + n + halfPi) / pi, 0.0, 1.0);

                    indirect = updateSectors(
                        frontBackHorizon.x,
                        frontBackHorizon.y,
                        0u
                    );

                    uint newlyVisible = indirect & ~occlusion;
                    float vis = float(bitCount(newlyVisible)) / float(sectorCount);

                    if(vis > 0.0){
                        vec3 sampleNormal = normalize(mat3(view) * GetWorldNormal(sampleUV));
                        vec3 sampleLight = texture(mainTex, sampleUV).rgb;

                        float ndl = clamp(dot(normal, horizonDir), 0.0, 1.0);
                        float ldn = clamp(dot(sampleNormal, -horizonDir), 0.0, 1.0);

                        lighting += vis * sampleLight * ndl * ldn;
                    }

                    occlusion |= indirect;
                }
            }

            visibility += 1.0 - float(bitCount(occlusion)) / float(sectorCount);
        }

        visibility /= sliceCount;
        visibility = pow(clamp(visibility, 0.0, 1.0), aoPower);

        lighting /= sliceCount;

        // HDR safety clamp (like Three.js)
        float lum = dot(lighting, vec3(0.2126, 0.7152, 0.0722));
        if(lum > giClamp) lighting *= giClamp / lum;

        vec3 gi = (lighting * giIntensity);// * texture(gAlbedoSpec, texCoord).rgb;
        fragColor = vec4(gi * visibility, visibility);
        //fragColor = vec4(vec3(visibility), 1);

        return;
        vec4 directLighting = texture(mainTex, texCoord);
        vec4 diffuse = texture(gAlbedoSpec, texCoord);
        fragColor = vec4((directLighting.rgb * visibility) + (diffuse.rgb * gi), directLighting.a);
    }
#endif
