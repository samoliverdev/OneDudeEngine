#if defined(VERTEX) && defined(VertexFunction)

layout(location = 0) in vec3 _pos;
layout(location = 1) in vec2 _texCoord;
layout(location = 2) in vec3 _normal;

out ShaderData outData;

void main(){
    outData.pos = _pos;
    outData.normal = _normal;
    outData.texCoord = _texCoord;
    gl_Position = VertexFunction(outData);
}
#endif

#if defined(FRAGMENT)

in ShaderData inData;
out vec4 fragColor;

void main(){
    fragColor = FragmentFunction(inData);
}
#endif
