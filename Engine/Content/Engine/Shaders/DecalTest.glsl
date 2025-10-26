#pragma BeginProperties
    Color4  color
    Texture2D mainTex White
    Float decalBlend 1 0 1
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    DrawType _ INSTANCING INSTANCINGMATRIX43

    CullFace NONE
    DepthMask False
    DepthTest ALWAYS
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
#pragma EndPassDef

#define Deferred

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
    Uniform mat4 decalWorldToLocal;
    Uniform float decalBlend;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)
//Texture2D(0, 2, gPosition, gPositionSampler)
Texture2D(0, 3, gNormal, gNormalSampler)
Texture2D(0, 4, gAlbedoSpec, gAlbedoSpecSampler)
Texture2D(0, 10, gDepth, gDepthSampler)

uniform int perDrawInt_1;

#if defined(VERTEX) && defined(MainPass)
    //flat out mat4 outDecalWorldToLocal;

    flat out vec4 vDecalInvRow0;
    flat out vec4 vDecalInvRow1;
    flat out vec4 vDecalInvRow2;
    flat out vec4 vDecalInvRow3;

    void main(){
        mat4 targetModelMatrix = GetModelMatrix();
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
        //OutPosition.z -= 0.001 * OutPosition.w;

        // Calculate camera-space  // LESS_EQUAL
        /*vec4 worldPos = targetModelMatrix * GetLocalPos();
        float viewZ = -(view * worldPos).z; // negative if camera looks down -Z
        float cameraDist = abs(viewZ);// Convert to positive distance
        float bias = clamp(0.0005 * (1.0 + 10.0 / cameraDist), 0.0001, 0.002);// Dynamic bias that grows when close to the camera
        OutPosition.z -= bias * OutPosition.w;// Push decal slightly toward the camera*/

        //outDecalWorldToLocal = inverse(targetModelMatrix);

        mat4 invModel = inverse(targetModelMatrix);
        vDecalInvRow0 = invModel[0];
        vDecalInvRow1 = invModel[1];
        vDecalInvRow2 = invModel[2];
        vDecalInvRow3 = invModel[3];
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    #include Engine/ShaderLibrary/Core.glsl
    #include Engine/ShaderLibrary/Common.glsl

    //flat in mat4 outDecalWorldToLocal;

    flat in vec4 vDecalInvRow0;
    flat in vec4 vDecalInvRow1;
    flat in vec4 vDecalInvRow2;
    flat in vec4 vDecalInvRow3;

    //Out(2) vec4 gAlbedo;
    layout(location = 1) out vec4 gAlbedo;

    void main(){
        //gAlbedo = vec4(1,1,1,1);
        //return;

        mat4 outDecalWorldToLocal = mat4(
            vDecalInvRow0,
            vDecalInvRow1,
            vDecalInvRow2,
            vDecalInvRow3
        );

        vec2 screenUV = gl_FragCoord.xy / vec2(textureSize(gAlbedoSpec, 0));
        vec3 worldPos = reconstructWorldPos(screenUV, texture(gDepth, screenUV).r, invProjection, invView);// texture(gPosition, screenUV).rgb;
        vec3 normal   = unpack_normal_octahedron(texture(gNormal, screenUV).rg); //texture(gNormal, screenUV).rgb;
        vec4 albedo   = texture(gAlbedoSpec, screenUV);

        if(perDrawInt_1 >= 0 && perDrawInt_1 != albedo.a) discard;

        // Transform world position into decal local space
        vec3 localPos = (outDecalWorldToLocal * vec4(worldPos, 1.0)).xyz;

        // Check if inside decal box
        if(any(greaterThan(abs(localPos), vec3(0.5)))) discard;

        // Compute UV inside decal box
        vec2 uv = localPos.xy + 0.5;

        vec4 decalColor = ToLinear(SampleTexture2D(mainTex, mainSampler, uv));

        // angle fade (optional)
        vec3 decalNormalWS = normalize((inverse(outDecalWorldToLocal) * vec4(0,0,1,0)).xyz);
        float angleFade = clamp(dot(decalNormalWS, normal), 0.0, 1.0);

        // fade near top/bottom
        float fade = 1;// * angleFade;

        vec3 finalAlbedo = mix(albedo.rgb, decalColor.rgb, decalBlend * decalColor.a * fade);

        gAlbedo.rgb = finalAlbedo;
        gAlbedo.a = 1;
    }
#endif