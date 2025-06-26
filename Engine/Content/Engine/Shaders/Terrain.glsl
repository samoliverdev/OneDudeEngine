#pragma BeginProperties
    Color4 color
    Texture2D splatmap Black
    Texture2D tex0 White
    Texture2D tex1 White
    Texture2D tex2 White
    Texture2D tex3 White
    Texture2D tex4 White
    Texture2D mainTex White
    Texture2D heightMap Back
    Texture2D heightMapNormal Back
    Texture2D normalMap Normal
    Texture2D emissionMap Black
    Color4 emissionColor
    Texture2D maskMap White
    Float occlusion 1 0 1
    Float metallic 0 0 1
    Float smoothness 0.5 0.0 1.0
    Float cutoff 0.5 0 1
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing false
    MultiCompile _ SKINNED
    MultiCompile Opaque Blend
    CullFace BACK
    DepthTest LESS
    Blend Off
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

#include Engine/ShaderLibrary/UniformsDef.glsl
#include Engine/ShaderLibrary/TexturesDef.glsl

BeginUniform(0, 0, Main)
    Uniform vec3 viewPos;
    Uniform float metersPerHeightfieldTexel;// = 1;
    Uniform vec2 uvOffset;// = vec2(0);
    Uniform vec2 heightmapTilling;// = vec2(1, 1);
    Uniform vec2 heightmapOffset;// = vec2(0, 0);
    Uniform vec2 texTilling;// = vec2(1, 1);
    Uniform float heightScale;
    Uniform vec4 color;// = vec4(1,1,1,1);
    Uniform float normalStrength;// = 1;
    Uniform vec4 emissionColor;// = vec4(0,0,0,0);
    Uniform float occlusion;// = 1;
    Uniform float metallic;// = 0;
    Uniform float smoothness;// = 0.5;
    Uniform float cutoff;// = 0.5;
EndUniform()

//#define USE_PERDRAW
uniform vec4 customData;

Texture2D(0, 6, heightMap, heightMapSampler)
Texture2D(0, 7, heightMapNormal, heightMapNormalSampler)
Texture2D(0, 8, heightMapNorth, heightMapNorthSampler)
Texture2D(0, 9, heightMapWest, heightMapWestSampler)
Texture2D(0, 10, mainTex, mainTexSampler)
Texture2D(0, 11, splatmap, splatmapSampler)
Texture2D(0, 12, tex0, tex0Sampler)
Texture2D(0, 13, tex1, tex1Sampler)
Texture2D(0, 14, tex2, tex2Sampler)
Texture2D(0, 15, tex3, tex3Sampler)
Texture2D(0, 16, tex4, tex4Sampler)
Texture2D(0, 17, normalMap, normalMapSampler)
Texture2D(0, 18, emissionMap, emissionMapSampler)
Texture2D(0, 19, maskMap, maskMapSampler)

#if defined(VERTEX) && defined(MainPass)
    out VsOut{
        vec3 pos;
        vec3 normal;
        vec2 texCoord;
        vec3 worldPos;
        vec3 worldNormal;
        mat3 normalMatrix;
        flat vec2 uvOffset_;
        mat3 TBN;
        mat4 targetModelMatrix;
    } vsOut;

    /*uniform sampler2D heightMap;
    uniform float heightScale;
    uniform vec3 viewPos;
    uniform float metersPerHeightfieldTexel = 1;
    uniform vec2 uvOffset = vec2(0);
    uniform vec2 heightmapTilling = vec2(1, 1);
    uniform vec2 heightmapOffset = vec2(0, 0);*/

    vec3 NormalStrength(vec3 In, float Strength){
        return vec3(In.rg * Strength, mix(1, In.b, clamp(Strength, 0, 1)));
    }

    vec3 filterNormalLod(vec2 uv){
        vec2 texSize = textureSize(heightMap, 0);
        vec2 texelSize = vec2(1.0 / texSize.x, 1.0 / texSize.y);
        
        /*float h0 = texture(heightMap, uv + ( vec2( 0,-1) * texelSize) ).r * heightScale;
        float h1 = texture(heightMap, uv + ( vec2(-1, 0) * texelSize) ).r * heightScale;
        float h2 = texture(heightMap, uv + ( vec2( 1, 0) * texelSize) ).r * heightScale;
        float h3 = texture(heightMap, uv + ( vec2( 0, 1) * texelSize) ).r * heightScale;
        return normalize(vec3(h1 - h2, 2, h0 - h3));*/

        float left = texture(heightMap, uv + ( vec2( -1,0) * texelSize) ).r * heightScale;
        float right = texture(heightMap, uv + ( vec2(1, 0) * texelSize) ).r * heightScale;
        float up = texture(heightMap, uv + ( vec2( 0, 1) * texelSize) ).r * heightScale;
        float down = texture(heightMap, uv + ( vec2( 0, -1) * texelSize) ).r * heightScale;
        return normalize(vec3(left - right, 2.0, up - down));
    }

    void main(){
        #ifdef USE_PERDRAW
        vec2 _heightmapOffset = vec2(customData.x, customData.y);
        #else
        vec2 _heightmapOffset = heightmapOffset;
        #endif

        mat4 targetModelMatrix = GetModelMatrix();
        vec3 localPos = GetLocalPos().xyz;
        //vec3 _normal = NormalStrength(filterNormalLod(texCoord * heightmapTilling + heightmapOffset), 1);
        vec3 _normal = normal;

        vec3 T = normalize(vec3(targetModelMatrix * vec4(tangents, 0.0)));
        vec3 B = normalize(vec3(targetModelMatrix * vec4(cross(tangents, _normal), 0.0)));
        vec3 N = normalize(vec3(targetModelMatrix * vec4(_normal, 0.0)));
        vsOut.TBN = mat3(T, B, N);

        vec2 uv = texCoord;
        //uv.y = 1 - uv.y;

        float height = texture(heightMap, uv * heightmapTilling + _heightmapOffset).r; // uv + uvOffset
        localPos.y = height * heightScale;

        vsOut.targetModelMatrix = targetModelMatrix;
        vsOut.pos = localPos;
        vsOut.normal = _normal;
        vsOut.texCoord = texCoord;
        vsOut.worldPos = vec3(targetModelMatrix * vec4(localPos, 1.0));
        //worldNormal = vec3(model * vec4(normal, 1.01));
        vsOut.worldNormal = mat3(transpose(inverse(targetModelMatrix))) * _normal; // for non-uniform scale objects
        vsOut.normalMatrix = mat3(transpose(inverse(targetModelMatrix)));

        gl_Position = projection * view * targetModelMatrix * vec4(localPos, 1.0);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    //uniform mat4 view;
    #include Engine/ShaderLibrary/Core.glsl
    #include Engine/ShaderLibrary/Common.glsl
    #include Engine/ShaderLibrary/Surface.glsl
    #include Engine/ShaderLibrary/Shadows.glsl
    #include Engine/ShaderLibrary/Light.glsl
    #include Engine/ShaderLibrary/BRDF.glsl
    #include Engine/ShaderLibrary/GI.glsl
    #include Engine/ShaderLibrary/Lighting.glsl

    in VsOut{
        vec3 pos;
        vec3 normal;
        vec2 texCoord;
        vec3 worldPos;
        vec3 worldNormal;
        mat3 normalMatrix;
        flat vec2 uvOffset_;
        mat3 TBN;
        mat4 targetModelMatrix;
    } fsIn;

    /*uniform vec3 viewPos;
    uniform float heightScale;
    uniform sampler2D heightMap;
    uniform sampler2D heightMapNormal;
    uniform sampler2D heightMapNorth;
    uniform sampler2D heightMapWest;
    uniform vec2 uvOffset = vec2(0);
    uniform sampler2D mainTex;
    uniform sampler2D splatmap;
    uniform sampler2D tex0;
    uniform sampler2D tex1;
    uniform sampler2D tex2;
    uniform sampler2D tex3;
    uniform sampler2D tex4;
    uniform vec4 color = vec4(1,1,1,1);
    uniform sampler2D normal;
    uniform float normalStrength = 1;
    uniform sampler2D emissionMap;
    uniform vec4 emissionColor = vec4(0,0,0,0);
    uniform sampler2D maskMap;
    uniform float occlusion = 1;
    uniform float metallic = 0;
    uniform float smoothness = 0.5;
    uniform float cutoff  = 0.5;*/

    out vec4 fragColor;

    vec3 NormalStrength(vec3 In, float Strength){
        return vec3(In.rg * Strength, mix(1, In.b, clamp(Strength, 0, 1)));
    }

    //Source: https://forum.unity.com/threads/calculate-vertex-normals-in-shader-from-heightmap.169871/
    vec3 filterNormalLod(vec2 uv){
        vec2 texSize = textureSize(heightMap, 0);
        vec2 texelSize = vec2(1.0 / texSize.x, 1.0 / texSize.y);
        
        /*float h0 = texture(heightMap, uv + ( vec2( 0,-1) * texelSize) ).r * heightScale;
        float h1 = texture(heightMap, uv + ( vec2(-1, 0) * texelSize) ).r * heightScale;
        float h2 = texture(heightMap, uv + ( vec2( 1, 0) * texelSize) ).r * heightScale;
        float h3 = texture(heightMap, uv + ( vec2( 0, 1) * texelSize) ).r * heightScale;
        return normalize(vec3(h1 - h2, 2, h0 - h3));*/

        float left = texture(heightMap, uv + ( vec2( -1,0) * texelSize) ).r * heightScale;
        float right = texture(heightMap, uv + ( vec2(1, 0) * texelSize) ).r * heightScale;
        float up = texture(heightMap, uv + ( vec2( 0, 1) * texelSize) ).r * heightScale;
        float down = texture(heightMap, uv + ( vec2( 0, -1) * texelSize) ).r * heightScale;
        return normalize(vec3(left - right, 2.0, up - down));
    }

    vec3 getNormalFromMap(vec2 uv, vec3 WorldPos0, mat4 targetModelMatrix){
        vec3 tangentNormal = texture(normalMap, uv).xyz * 2.0 - 1.0;

        vec3 Q1 = dFdx(WorldPos0);
        vec3 Q2 = dFdy(WorldPos0);
        vec2 st1 = dFdx(uv);
        vec2 st2 = dFdy(uv);
        vec3 Normal0 = /*mat3(transpose(inverse(targetModelMatrix))) **/ filterNormalLod(uv);
        
        /*vec3 N = normalize(Normal0);
        vec3 T = normalize(Q1*st2.t - Q2*st1.t);
        vec3 B = -normalize(cross(N, T));*/

        vec3 T = normalize(vec3(targetModelMatrix * vec4(Q1*st2.t - Q2*st1.t, 0.0)));
        vec3 N = normalize(vec3(targetModelMatrix * vec4(Normal0, 0.0)));
        vec3 B = normalize(vec3(targetModelMatrix * vec4(cross(Q1*st2.t - Q2*st1.t, Normal0), 0.0)));

        mat3 TBN = mat3(T, B, N);

        return normalize(TBN * tangentNormal);
    }

    //
    vec3 getNormalFromMap2(vec2 uv, vec3 WorldPos0, mat4 targetModelMatrix){
        vec3 tangentNormal = texture(normalMap, uv).xyz * 2.0 - 1.0;

        vec3 Normal0 = filterNormalLod(uv);
        //Source: https://www.reddit.com/r/opengl/comments/184zjg8/can_i_calculate_tangent_space_based_on_the_height/
        vec3 Tangent = normalize(cross(vec3(1, 0, 0), Normal0));
        if(abs(dot(Normal0, vec3(0, 1, 0))) < 0.999){
            Tangent = normalize(cross(vec3(0, 1, 0), Normal0));
        }
        
        vec3 N = normalize(Normal0);
        vec3 T = normalize(Tangent);
        vec3 B = -normalize(cross(N, T));

        /*vec3 T = normalize(vec3(targetModelMatrix * vec4(Tangent, 0.0)));
        vec3 N = normalize(vec3(targetModelMatrix * vec4(Normal0, 0.0)));
        vec3 B = normalize(vec3(targetModelMatrix * vec4(cross(Tangent, Normal0), 0.0)));*/

        mat3 TBN = mat3(T, B, N);
        return normalize(TBN * tangentNormal);
    }

    vec3 GetEmission(vec2 baseUV){
        vec4 map = texture(emissionMap, baseUV);
        return map.rgb * emissionColor.rgb;
    }

    vec4 GetMask(vec2 baseUV){
        return texture(maskMap, baseUV);
    }

    float GetMetallic(vec2 baseUV){
        float _metallic = metallic;
        _metallic *= GetMask(baseUV).r;
        return _metallic;
    }

    float GetSmoothness(vec2 baseUV){
        float _smoothness = smoothness;
        _smoothness *= GetMask(baseUV).a;
        return _smoothness;
    }

    float GetOcclusion(vec2 baseUV){
        return 1.0;
        //return GetMask(baseUV).g;

        /*float strength = occlusion;
        float _occlusion = GetMask(baseUV).g;
        _occlusion = mix(_occlusion, 1.0, strength);
        return _occlusion;*/
    }

    vec3 GetNormal(mat3 TBN, vec2 uv){
        vec3 n = texture(heightMapNormal, uv).xyz;
        //return fsIn.normalMatrix * n;

        n = n * 2.0 - 1.0;
        //n.xy *= normalStrength;
        n = normalize(n);
        return normalize(TBN * n);
    }

    //uniform vec2 heightmapTilling = vec2(1, 1);
    //uniform vec2 heightmapOffset = vec2(0, 0);

    void main(){
        #ifdef USE_PERDRAW
        vec2 _heightmapOffset = vec2(customData.x, customData.y);
        #else
        vec2 _heightmapOffset = heightmapOffset;
        #endif

        vec4 splatmap = texture(splatmap, fsIn.texCoord * heightmapTilling + _heightmapOffset);

        vec2 baseUV = fsIn.texCoord * texTilling;

        //vec4 base = texture(mainTex, fsIn.texCoord + uvOffset);
        vec4 base = texture(mainTex, baseUV);
        base = base * color;
        //base = color;

        base = mix(
            texture(tex0, baseUV),
            texture(tex1, baseUV),
            splatmap.r
        );
        base = mix(
            base,
            texture(tex2, baseUV),
            splatmap.g
        );
        base = mix(
            base,
            texture(tex3, baseUV),
            splatmap.b
        );
        base = mix(
            base,
            texture(tex4, baseUV),
            splatmap.a
        );

        //float height = texture(heightMap, fsIn.texCoord + fsIn.uvOffset_).r * 1;
        //base = vec4(height, height, height, 1);
        //base = vec4((fsIn.texCoord + uvOffset), 0, 1);

        Surface surface;
        surface.position = fsIn.worldPos;
        surface.normal = normalize(fsIn.worldNormal);
        //surface.normal = NormalStrength(filterNormalLod(fsIn.texCoord * heightmapTilling + heightmapOffset), 1);
        //surface.normal = NormalStrength(GetNormal(fsIn.TBN, fsIn.texCoord * heightmapTilling + heightmapOffset), 1);
        surface.normal = getNormalFromMap2(fsIn.texCoord * heightmapTilling + _heightmapOffset, fsIn.pos, fsIn.targetModelMatrix);
        surface.viewDirection = normalize(viewPos - fsIn.worldPos);
        surface.depth = -(view * vec4(fsIn.worldPos, 1)).z;
        surface.color = base.rgb;
        surface.alpha = base.a;
        surface.occlusion = GetOcclusion(fsIn.texCoord);
        surface.metallic = GetMetallic(fsIn.texCoord);
        surface.smoothness = GetSmoothness(fsIn.texCoord);
        
        BRDF brdf = GetBRDF(surface);
        GI gi = GetGI(surface, brdf);
        vec3 color = GetLighting(surface, brdf, gi);
        color += GetEmission(fsIn.texCoord);
        fragColor = vec4(color, surface.alpha);

        if(base.a < cutoff) discard;
    }
#endif