#pragma BeginProperties
    Color4 color
    float roughness = 0.2
    float metalness = 1.0
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    DrawType _ SKINNED INSTANCING INSTANCINGMATRIX43
#pragma EndPassDef

#pragma GLSL
    #include Engine/ShaderLibrary/Base.glsl
    #include Engine/ShaderLibrary/Vertex.glsl

    BeginUniform(0, 0, Main)
        Uniform float roughness;
        Uniform float metalness;
        Uniform float levels;
        Uniform vec3  cameraPos;
    EndUniform()

    TextureCube(0, 1, mainTex, mainSampler)

    //------------------VERTEX------------------
    #if defined(VERTEX) && defined(MainPass)
    Out(0) vec3 WorldPos;
    Out(1) vec3 WorldNormal;

    void main(){
        mat4 model = GetModelMatrix();

        vec4 worldPos = model * GetLocalPos();
        WorldPos = worldPos.xyz;

        WorldNormal = normalize(mat3(model) * GetLocalNormal());

        OutPosition = projection * view * worldPos;
    }
    #endif

    //------------------FRAGMENT------------------
    #if defined(FRAGMENT) && defined(MainPass)
    #include Engine/ShaderLibrary/Core.glsl

    In(0) vec3 WorldPos;
    In(1) vec3 WorldNormal;

    Out(0) vec4 FragColor;

    void main(){
        vec4 color = vec4(1, 1, 1, 1);
        
        vec3 N = normalize(WorldNormal);
        vec3 V = normalize(cameraPos - WorldPos);

        // Reflection vector
        vec3 R = reflect(-V, N);

        // Roughness → mip level
        float maxMip = float(levels - 1); // 8.0; // change depending on your cube mip count
        //float perceptualRoughness = roughness * roughness;
        //float lod = perceptualRoughness * maxMip;
        float lod = roughness * maxMip;
        //float lod = pow(roughness, 1.5) * maxMip;
        //float lod = pow(roughness, 1.5) * (maxMip - 1.0);

        vec3 envColor = (SampleTextureCubeLod(mainTex, mainSampler, R, lod).rgb);

        /*vec3 envColor = vec3(0.0);
        envColor += (SampleTextureCubeLod(mainTex, mainSampler, R, lod).rgb); //textureLod(..., R, lod);
        envColor += (SampleTextureCubeLod(mainTex, mainSampler, R + N * 0.1, lod).rgb); //textureLod(..., R + N * 0.1, lod);
        envColor += (SampleTextureCubeLod(mainTex, mainSampler, R - N * 0.1, lod).rgb); //textureLod(..., R - N * 0.1, lod);
        envColor /= 3.0;*/

        // ----- Fresnel (Schlick) -----

        vec3 F0 = mix(vec3(0.04), color.rgb, metalness);

        float NdotV = max(dot(N, V), 0.0);
        vec3 F = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

        vec3 specular = envColor * F;

        // Fake irradiance (diffuse IBL)
        vec3 irradiance = (SampleTextureCubeLod(mainTex, mainSampler, N, maxMip).rgb);

        // Lambert diffuse
        vec3 kd = (1.0 - F) * (1.0 - metalness);

        vec3 diffuse = irradiance * kd * color.rgb;

        vec3 finalColor = diffuse + specular;
        FragColor = vec4(finalColor, 1.0);
    }
    #endif
#pragma EndGLSL