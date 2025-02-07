#pragma BeginPassDef
    Name MainPass
    CullFace NONE
    DepthMask False
    Blend SRC_ALPHA ONE_MINUS_SRC_ALPHA
#pragma EndPassDef

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
    uniform vec3 color = vec3(0, 0, 1);
    uniform float alpha = 0.5;

    out vec4 outColor;

    void main(){
        outColor = vec4(color.xyz, alpha);
    }
#endif