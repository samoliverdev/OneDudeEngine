BeginPass
    #pragma Name MainPass
    #pragma DepthTest ALWAYS

    #include Engine/ShaderLibrary/Base.glsl
    #include Engine/ShaderLibrary/Vertex.glsl

    #if defined(OpenGL_API)
        uniform vec3 color;
    #else
    BeginUniform(0, 0, Main)
        Uniform vec3 color;
    EndUniform()
    #endif

    BeginVertex
        layout(location = 0) in vec3 position;

        //uniform mat4 model;
        //uniform mat4 view;
        //uniform mat4 projection;

        void main() {
            gl_Position = projection * view * model * vec4(position, 1);
        }
    EndVertex

    BeginFrag
        out vec4 outColor;

        /*#if defined(OpenGL_API)
        uniform vec3 color;
        #endif*/

        void main(){
            /*#if !defined(OpenGL_API)
            vec3 color = vec3(0, 0, 1);
            #endif*/
            
            float alpha = 1.0;
            outColor = vec4(color.xyz, alpha);
        }
    EndFrag
EndPass