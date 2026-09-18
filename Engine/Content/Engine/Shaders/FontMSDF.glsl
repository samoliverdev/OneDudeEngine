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
#include Engine/ShaderLibrary/Vertex.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
    Uniform float pxRange;
EndUniform()
Texture2D(0, 1, mainTex, mainSampler)

#if defined(VERTEX)
    Out(0) vec2 _texCoord;

    void main() {
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT)
    In(0) vec2 _texCoord;
    Out(0) vec4 fragColor;

    float median(float r, float g, float b) {
        return max(min(r, g), min(max(r, g), b));
    }

    vec2 sqr(vec2 x) { return x*x; } // squares vector components

    float screenPxRange(){
        vec2 unitRange = vec2(pxRange)/vec2(textureSize(mainTex, 0));
        vec2 screenTexSize = inversesqrt(sqr(dFdx(_texCoord))+sqr(dFdy(_texCoord)));// If inversesqrt is not available, use vec2(1.0)/sqrt
        return max(0.5*dot(unitRange, screenTexSize), 1.0);// Can also be approximated as screenTexSize = vec2(1.0)/fwidth(texCoord);
    }

    /*float screenPxRange(){
        vec2 unitRange = vec2(pxRange) / vec2(textureSize(mainTex, 0));
        vec2 screenTexSize = vec2(1.0) / fwidth(texCoord);
        return max(0.5 * dot(unitRange, screenTexSize), 1.0);
    }*/

    void main(){
        vec3 msd = texture(mainTex, _texCoord).rgb;
        float sd = median(msd.r, msd.g, msd.b);
        float screenPxDistance = screenPxRange()*(sd - 0.5);
        float alpha = clamp(screenPxDistance + 0.5, 0.0, 1.0);
        
        fragColor = vec4(color.rgb, alpha);

        //fragColor = vec4(msd, 1.0);
        //return;
    }
#endif