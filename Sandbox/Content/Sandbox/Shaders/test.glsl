#pragma BeginPassDef
    Name MainPass
    CullFace NONE
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

#if defined(VERTEX) && defined(MainPass)
    layout (location = 0) in vec3 vPosition;

    out vec3 pos; 
    void main() {
        pos = vPosition;
        gl_Position = vec4(pos, 1);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    in vec3 pos;
    out vec4 color;
    void main(){
        color = vec4(pos.xyz, 1);
    }
#endif