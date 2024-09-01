#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
out vec3 pos;

uniform mat4 projection;
uniform mat4 view;

void main(){
    pos = _pos;
    gl_Position = projection * view * vec4(pos, 1.0); 
}
#endif

#if defined(FRAGMENT)
out vec4 FragColor;
in vec3 pos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v){
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main(){
    vec2 uv = SampleSphericalMap(normalize(pos)); // make sure to normalize localPos
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}
#endif