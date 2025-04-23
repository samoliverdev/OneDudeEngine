BeginPass
    #pragma Name MainPass

    #include Engine/ShaderLibrary/Base.glsl

    BeginUniform(0, 0, Main)
        Uniform mat4 projection2;
        Uniform mat4 view2;
    EndUniform()
    Texture2D(0, 1, equirectangularMap, equirectangularMapSampler)

    BeginVertex
    layout (location = 0) in vec3 _pos;
    out vec3 pos;

    void main(){
        pos = _pos;
        gl_Position = projection2 * view2 * vec4(pos, 1.0); 
    }
    EndVertex

    BeginFrag
    out vec4 FragColor;
    in vec3 pos;

    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 SampleSphericalMap(vec3 v){
        vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
        uv *= invAtan;
        uv += 0.5;
        return uv;
    }

    void main(){
        vec2 uv = SampleSphericalMap(normalize(pos)); // make sure to normalize localPos
        vec3 color = texture(equirectangularMap, uv).rgb;
        FragColor = vec4(color, 1.0);
    }
    EndFrag
EndPass