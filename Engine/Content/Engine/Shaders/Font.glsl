#pragma BeginPassDef
    Name MainPass
    CullFace NONE
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
    DepthMask True
    DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)

#if defined(MainPass)
    #if defined(VERTEX)
    layout(location = 0) in vec3 _pos;
    layout(location = 1) in vec2 _texCoord;
    out vec2 texCoord;

    BeginUniform(2, 0, CamDraw)
        Uniform mat4 projection;
        Uniform mat4 view;
    EndUniform()

    uniform mat4 model;

    void main(){
        texCoord = _texCoord;
        gl_Position = projection * view * model * vec4(_pos, 1.0);
    }
    #endif

    #if defined(FRAGMENT)
    in vec2 texCoord;
    out vec4 fragColor;

    float median(float r, float g, float b) {
        return max(min(r, g), min(max(r, g), b));
    }

    float screenPxRange(){
        const float pxRange = 2.0; // set to distance field's pixel range
        vec2 unitRange = vec2(pxRange) / vec2(textureSize(mainTex, 0));
        vec2 screenTexSize = vec2(1.0) / fwidth(texCoord);
        return max(0.5 * dot(unitRange, screenTexSize), 1.0);
    }

    void main(){
        /*vec4 sampled = vec4(1.0, 1.0, 1.0, texture(mainTex, texCoord).r);
        fragColor = vec4(color.rgb, 1.0) * sampled;
        fragColor = vec4(1, 1, 1, 1) * sampled;*/

        vec4 bgColor = vec4(1, 0, 0, 1);
        vec4 fgColor = vec4(1, 0, 0, 1);
        vec3 msd = texture(mainTex, texCoord).rgb;
        float sd = median(msd.r, msd.g, msd.b);
        float screenPxDistance = screenPxRange()*(sd - 0.5);
        float alpha = clamp(screenPxDistance + 0.5, 0.0, 1.0);
        fragColor = vec4(fgColor.rgb * alpha, alpha);
    }
    #endif
#endif