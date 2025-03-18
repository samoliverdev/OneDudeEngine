#pragma BeginPassDef
    Name MainPass
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

#if defined(VERTEX) && defined(MainPass)
    layout (location = 0) in vec3 position;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(position, 1);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    out vec4 outColor;

    #if defined(OpenGL_API)
    uniform vec3 color;
    #endif

    void main(){
        #if !defined(OpenGL_API)
        vec3 color = vec3(0, 0, 1);
        #endif
        
        float alpha = 1.0;

        outColor = vec4(color.xyz, alpha);
    }
#endif