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
uniform float exposure = 2;

in vec3 pos;
in vec2 texCoord;

out vec4 fragColor;

const float offset = 1.0 / 300.0; 

void main() {
    const float gamma = 2.2;
    vec3 hdrColor = texture(mainTex, texCoord).rgb;
  
    // reinhard tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    // gamma correction 
    //mapped = pow(mapped, vec3(1.0 / gamma));
  
    fragColor = vec4(mapped, 1.0);
}
#endif