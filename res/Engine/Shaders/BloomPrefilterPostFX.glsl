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

uniform vec4 _BloomThreshold;

vec4 GetSource(vec2 uv){
    return texture(mainTex, uv);
}

vec3 ApplyBloomThreshold(vec3 color){
    /*float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > _BloomThreshold.x) return color;
    return vec3(0.0);*/
	
    float brightness = max(max(color.r, color.g), color.b); //Max3(color.r, color.g, color.b);
	float soft = brightness + _BloomThreshold.y;
	soft = clamp(soft, 0.0, _BloomThreshold.z);
	soft = soft * soft * _BloomThreshold.w;
	float contribution = max(soft, brightness - _BloomThreshold.x);
	contribution /= max(brightness, 0.00001);
	return color * contribution;
    //return mix(vec3(0), color, contribution);
    
}

vec4 BloomPrefilterPassFragment(){
	vec3 color = ApplyBloomThreshold(GetSource(texCoord).rgb);
	return vec4(color, 1.0);
}

void main(){
    fragColor = BloomPrefilterPassFragment();
}
#endif