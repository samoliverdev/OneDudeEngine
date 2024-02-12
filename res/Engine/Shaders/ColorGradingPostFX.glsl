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

uniform vec4 _ColorAdjustments;
uniform vec4 _ColorFilter;

vec3 ColorGradePostExposure(vec3 color){
	return color * _ColorAdjustments.x;
}

#define ACEScc_MIDGRAY 0.4135884
vec3 ColorGradingContrast(vec3 color){
	return (color - ACEScc_MIDGRAY) * _ColorAdjustments.y + ACEScc_MIDGRAY;
}

vec3 ColorGradeColorFilter(vec3 color) {
	return color * _ColorFilter.rgb;
}

// Source: https://github.com/Unity-Technologies/FPSSample/blob/master/Packages/com.unity.postprocessing/PostProcessing/Shaders/Colors.hlsl

#define EPSILON 0.00001

vec3 RgbToHsv(vec3 c){
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = EPSILON;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 HsvToRgb(vec3 c){
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0, 1), c.y);
}

float RotateHue(float value, float low, float hi){
    return (value < low)
            ? value + hi
            : (value > hi)
                ? value - hi
                : value;
}

vec3 ColorGradingHueShift(vec3 color){
	color = RgbToHsv(color);
	float hue = color.x + _ColorAdjustments.z;
	color.x = RotateHue(hue, 0.0, 1.0);
	return HsvToRgb(color);
}

float Luminance(vec3 linearRgb){
    return dot(linearRgb, vec3(0.2126729, 0.7151522, 0.0721750));
}

vec3 ColorGradingSaturation(vec3 color){
	float luminance = Luminance(color);
	return (color - luminance) * _ColorAdjustments.w + luminance;
}

void main(){
	vec3 color = texture(mainTex, texCoord).rgb;
	color = min(color, 60.0);
    color = ColorGradePostExposure(color);
    color = ColorGradingContrast(color);
    color = ColorGradeColorFilter(color);
    color = max(color, 0.0);
    color = ColorGradingSaturation(color);
	color = max(color, 0.0);

    fragColor = vec4(color, 1);
}
#endif