//Source: https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/

#pragma BeginPassDef
    Name Pass0
	CullFace BACK
	DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name Pass1
	CullFace BACK
	DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name Pass2
	CullFace BACK
	DepthTest DISABLE
    Blend ONE ONE
#pragma EndPassDef

#pragma BeginPassDef
    Name Pass3
	CullFace BACK
	DepthTest DISABLE
#pragma EndPassDef

#pragma BeginPassDef
    Name Pass4
	CullFace BACK
	DepthTest DISABLE
#pragma EndPassDef


#include Engine/ShaderLibrary/Base.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 _filter;
    Uniform float intensity;
    Uniform float threshold;
    Uniform float softThreshold;
EndUniform()
Texture2D(0, 0, mainTex, mainTexSampler)
Texture2D(0, 0, sourceTex, sourceTexSampler)

#if defined(VERTEX)
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

#if defined(FRAGMENT)
	in vec3 pos;
	in vec2 texCoord;
	out vec4 fragColor;

    /*vec3 Prefilter(vec3 c){
        float brightness = max(c.r, max(c.g, c.b));
        float contribution = max(0, brightness - _filter.x);
        contribution /= max(brightness, 0.00001);
        return c * contribution;
    }*/

    vec3 Prefilter(vec3 c){
        float brightness = max(c.r, max(c.g, c.b));
        float soft = brightness - _filter.y;
        soft = clamp(soft, 0, _filter.z);
        soft = soft * soft * _filter.w;
        float contribution = max(soft, brightness - _filter.x);
        contribution /= max(brightness, 0.00001);
        return c * contribution;
    }

    vec3 Sample(vec2 uv){
        return texture(mainTex, uv).rgb;
    }

    vec3 SampleBox(vec2 uv, float delta){
        vec2 texelSize = 1.0 / vec2(textureSize(mainTex, 0));

        vec2 offset = texelSize * vec2(-delta, delta);

        vec3 s =
            Sample(uv + vec2(-offset.x, -offset.y)) +
            Sample(uv + vec2( offset.x, -offset.y)) +
            Sample(uv + vec2(-offset.x,  offset.y)) +
            Sample(uv + vec2( offset.x,  offset.y));

        return s * 0.25;
    }

    #if defined(Pass0)
	void main(){
		fragColor = vec4(Prefilter(SampleBox(texCoord, 1)), 1); 
	}
    #endif

    #if defined(Pass1)
	void main(){
		fragColor = vec4(SampleBox(texCoord, 1), 1);
	}
    #endif

    #if defined(Pass2)
	void main(){
		fragColor = vec4(SampleBox(texCoord, 0.5), 1);
	}
    #endif

    #if defined(Pass3)
	void main(){
		vec4 c = texture(sourceTex, texCoord);
		c.rgb += intensity * SampleBox(texCoord, 0.5);
        fragColor = c;
	}
    #endif

    #if defined(Pass4)
	void main(){
        fragColor = vec4(intensity * SampleBox(texCoord, 0.5), 1);
	}
    #endif
    
#endif