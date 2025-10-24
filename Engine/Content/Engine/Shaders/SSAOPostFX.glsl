#pragma BeginPassDef
    Name MainPass
    CullFace BACK
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Common.glsl

BeginUniform(0, 0, Main)
    Uniform vec3 viewPos;
    Uniform int lightIndex;
    Uniform float screenWidth; 
    Uniform float screenHeight;
EndUniform()

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
    Uniform mat4 invProjection;
    Uniform mat4 invView;
EndUniform()

Texture2D(0, 4, texNoise, texNoiseSampler)
Texture2D(0, 5, mainTex, mainTexSampler)
Texture2D(0, 6, gPosition, gPositionSampler)
Texture2D(0, 7, gNormal, gNormalSampler)
Texture2D(0, 8, gAlbedoSpec, gAlbedoSpecSampler)
Texture2D(0, 9, gEmission, gEmissionSampler)
Texture2D(0, 10, gOther, gOtherSampler)
Texture2D(0, 12, gDepth, gDepthSampler)

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

    uniform vec4 samples[64]; // Sample kernel

    /*uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 invProjection;
    uniform mat4 invView;*/

    uniform float intensity = 0.5;
    uniform float radius = 0.5;
    uniform float bias = 0.025;
    int kernelSize = 64;

    uniform vec2 noiseScale;

    float rand(vec2 co) {
        return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
    }

    void main(){
        float depth = texture(gDepth, texCoord).r;
        //if(depth >= 1.0) discard;

        vec3 fragPos = reconstructWorldPos(texCoord, texture(gDepth, texCoord).r, invProjection, invView); //texture(gPosition, texCoord).rgb;
        vec3 normal = unpack_normal_octahedron(texture(gNormal, texCoord).rg); //texture(gNormal, texCoord).rgb;
        
        fragColor = vec4(normal, 1);
        //return;

        vec3 randomVec = normalize(texture(texNoise, texCoord * noiseScale).xyz);
        fragColor = vec4(randomVec, 1);
        //return;

        /*vec2 noiseSeed = gl_FragCoord.xy / screenSize;
        randomVec = normalize(vec3(
            rand(noiseSeed),
            rand(noiseSeed + vec2(1.0, 0.0)),
            rand(noiseSeed + vec2(0.0, 1.0))
        ) * 2.0 - 1.0);*/

        fragPos = (view * vec4(fragPos, 1)).xyz;
        normal = normalize(mat3(view) * normal);  // Or use normal matrix //normalize((view * vec4(normal, 0)).xyz);

        //fragColor = vec4(normal, 1);
        //return;

        vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
        vec3 bitangent = cross(normal, tangent);
        mat3 TBN = mat3(tangent, bitangent, normal);
    
        float occlusion = 0.0;
        for(int i = 0; i < kernelSize; ++i){
            // get sample position
            vec3 samplePos = TBN * samples[i].xyz; // from tangent to view-space
            samplePos = fragPos + samplePos * radius; 
            
            // project sample position (to sample texture) (to get position on screen/texture)
            vec4 offset = vec4(samplePos, 1.0);
            offset = projection * offset; // from view to clip-space
            offset.xyz /= offset.w; // perspective divide
            offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
            
            // get sample depth
            //float sampleDepth = (view * vec4(texture(gPosition, offset.xy).xyz, 1)).z; //texture(gPosition, offset.xy).z; // get depth value of kernel sample
            float sampleDepth = (view * vec4(reconstructWorldPos(offset.xy, texture(gDepth, offset.xy).r, invProjection, invView), 1)).z;
            
            // range check & accumulate
            float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
            occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;           
        }
        occlusion = 1.0 - (occlusion / float(kernelSize));

        if(depth >= 1) occlusion = 1;

        occlusion = pow(occlusion, intensity);

        vec3 color = texture(mainTex, texCoord).rgb;
        fragColor = vec4(occlusion, occlusion, occlusion, 1);
        fragColor = vec4(color * occlusion, 1);
    }
#endif