BeginPass
    #pragma Name MainPass
    #pragma DepthTest DISABLE

    #include Engine/ShaderLibrary/Base.glsl

    BeginUniform(0, 0, Main)
        Uniform float option;
    EndUniform()
    Texture2D(0, 1, mainTex, mainSampler)

    BeginVertex
    layout (location = 0) in vec3 _pos;
    layout (location = 1) in vec2 _texCoord;

    out vec3 pos;
    out vec2 texCoord;

    void main() {
        pos = _pos;
        texCoord = _texCoord;
        gl_Position = vec4(pos, 1.0);
    }
    EndVertex

    #include Engine/ShaderLibrary/Core.glsl

    BeginFrag
    //uniform sampler2D mainTex;
    //uniform float option;

    in vec3 pos;
    in vec2 texCoord;
    out vec4 fragColor;

    vec3 ApplyDithering(vec3 color, vec2 uv) {
        float ditheringNoise = fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
        return color + (ditheringNoise * 0.01);  // Add slight noise
    }

    float Dither(vec2 fragCoord) {
        return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 0.001;
        //return fract(sin(dot(fragCoord, vec2(12.9898, 78.233))) * 43758.5453) * 0.003;
    }

    void main() {
        fragColor = texture(mainTex, texCoord);
        fragColor.rgb = ApplyGamaCorrection(fragColor.rgb);// + vec3(Dither(gl_FragCoord.xy));
        //fragColor.rgb = ApplyDithering(fragColor.rgb, texCoord);
        //fragColor.rgb = clamp(fragColor.rgb, 0.0, 1.0);
    }
    EndFrag
EndPass