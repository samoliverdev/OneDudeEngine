varying vec3 _pos;
varying vec3 _normal;
varying vec2 _texCoord;

#if defined(VERTEX)
#include Engine/ShaderLibrary/Vertex.glsl

vec4 VertexFunc(mat4 projection, mat4 view, mat4 model, vec4 pos);

void main(){
    _pos = pos;
    _normal = normal;
    _texCoord = texCoord;

    mat4 targetModelMatrix = GetModelMatrix();
    gl_Position = VertexFunc(projection, view, targetModelMatrix, GetLocalPos());
}
#endif

#if defined(FRAGMENT)
out vec4 fragColor;

vec4 FragmentFunc();

void main(){
    vec4 texColor = FragmentFunc();
    if(texColor.a < 0.1) discard;
    fragColor = texColor;
}
#endif