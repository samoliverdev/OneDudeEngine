#pragma BeginPassDef
    Name MainPass
	DepthTest DISABLE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

Texture2D(0, 0, mainTex, mainTexSampler)

#if defined(VERTEX) && defined(MainPass)
	Out(0) vec2 _texCoord;

    void main() {
        mat4 targetModelMatrix = GetModelMatrix();
        _texCoord = texCoord.xy;
        OutPosition = projection * view * targetModelMatrix * GetLocalPos();
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
	In(0) vec2 _texCoord;
	Out(0) vec4 fragColor;

	vec2 GetSourceTexelSize(){
		return vec2(1.0) / vec2(textureSize(mainTex, 0));
	}

	vec4 GetSource(vec2 uv){
		return texture(mainTex, uv);
	}

	vec4 BloomVerticalPassFragment(){
		vec4 color = vec4(0.0);
		float offsets[5] = float[5](
			//-4.0, -3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0
			-3.23076923, -1.38461538, 0.0, 1.38461538, 3.23076923
		);
		float weights[5] = float[5](
			//0.01621622, 0.05405405, 0.12162162, 0.19459459, 0.22702703,
			//0.19459459, 0.12162162, 0.05405405, 0.01621622
			0.07027027, 0.31621622, 0.22702703, 0.31621622, 0.07027027
		);
		for(int i = 0; i < 5; i++){
			float offset = offsets[i] * GetSourceTexelSize().y;
			color += GetSource(_texCoord + vec2(0.0, offset)) * weights[i];
		}
		return color; //vec4(color, 1.0);
	}

	void main(){
		fragColor = BloomVerticalPassFragment();
	}
#endif