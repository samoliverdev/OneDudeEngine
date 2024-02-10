#version 330 core

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
uniform sampler2D mainTex;

in vec3 pos;
in vec2 texCoord;

out vec4 fragColor;

vec2 GetSourceTexelSize(){
	return 1 / textureSize(mainTex, 0);
}

vec4 GetSource(vec2 uv){
    return texture(mainTex, uv);
}

vec4 BloomHorizontalPassFragment(){
	vec3 color = vec3(0.0);
	float offsets[] = {
		-4.0, -3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0
	};
	float weights[] = {
		0.01621622, 0.05405405, 0.12162162, 0.19459459, 0.22702703,
		0.19459459, 0.12162162, 0.05405405, 0.01621622
	};
	for(int i = 0; i < 9; i++) {
		float offset = offsets[i] * 2.0 * GetSourceTexelSize().x;
		color += GetSource(texCoord + vec2(offset, 0.0)).rgb * weights[i];
	}
	return vec4(color, 1.0);
}

void main(){
    fragColor = BloomHorizontalPassFragment();
}
#endif