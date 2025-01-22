#version 330 core

#pragma BeginProperties
    Color4 color
#pragma EndProperties

#pragma SupportInstancing true
#pragma MultiCompile _ SKINNED INSTANCING

#if defined(VERTEX)
#include Engine/ShaderLibrary/Vertex.glsl

out vec2 _texCoord;

void main(){
    //mat4 targetModelMatrix = (useInstancing >= 1.0 ? modelInstancing : model);
    mat4 targetModelMatrix = GetModelMatrix();

    _texCoord = texCoord;

    //gl_Position = projection * view * model * vec4(pos, 1.0);
    gl_Position = projection * view * targetModelMatrix * GetLocalPos();
}
#endif

#if defined(FRAGMENT)
uniform sampler2D mainTex;
uniform vec4 color;// = vec4(1,1,1,1);
//uniform test{ vec4 color; };

in vec2 _texCoord;

out vec4 fragColor;

void main(){
    vec4 texColor = texture(mainTex, _texCoord);
    if(texColor.a < 0.1) discard;
    fragColor = texColor * color;
}
#endif