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

#include Engine/ShaderLibrary/Base.glsl

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

    vec4 Sample(vec2 uv){
        return texture(mainTex, uv);
    }

    vec4 SampleBox(vec2 uv, float delta){
        vec2 texelSize = 1.0 / vec2(textureSize(mainTex, 0));

        vec2 offset = texelSize * vec2(-delta, delta);

        vec4 s =
            Sample(uv + vec2(-offset.x, -offset.y)) +
            Sample(uv + vec2( offset.x, -offset.y)) +
            Sample(uv + vec2(-offset.x,  offset.y)) +
            Sample(uv + vec2( offset.x,  offset.y));

        return s * 0.25;
    }


    #if defined(Pass0)
	void main(){
		fragColor = SampleBox(texCoord, 1);
	}
    #endif

    #if defined(Pass1)
	void main(){
		fragColor = SampleBox(texCoord, 0.5);
	}
    #endif
    
#endif