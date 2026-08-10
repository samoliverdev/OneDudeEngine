#pragma BeginProperties
    Color4 color
    Float pxRange 2
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    CullFace NONE
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
    DepthMask True
    DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name MainPass3D
    CullFace NONE
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
    DepthMask True
    DepthTest LESS
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Core.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
    Uniform float pxRange;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)

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

vec2 sqr(vec2 x) { return x*x; } // squares vector components

float screenPxRange(){
    vec2 unitRange = vec2(pxRange)/vec2(textureSize(mainTex, 0));
    vec2 screenTexSize = inversesqrt(sqr(dFdx(texCoord))+sqr(dFdy(texCoord)));// If inversesqrt is not available, use vec2(1.0)/sqrt
    return max(0.5*dot(unitRange, screenTexSize), 1.0);// Can also be approximated as screenTexSize = vec2(1.0)/fwidth(texCoord);
}

/*float screenPxRange(){
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(mainTex, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(texCoord);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}*/

void main(){
    vec3 msd = texture(mainTex, texCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange()*(sd - 0.5);
    float alpha = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    
    fragColor = vec4(color.rgb, alpha);

    //fragColor = vec4(msd, 1.0);
    //return;
}
#endif