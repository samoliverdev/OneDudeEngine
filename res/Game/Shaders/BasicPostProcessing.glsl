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
uniform float option;

in vec3 pos;
in vec2 texCoord;

out vec4 fragColor;

const float offset = 1.0 / 300.0; 

void main() {
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right    
    );

    fragColor = texture(mainTex, texCoord) * 2;
    
    if(option == 1){
        fragColor = vec4(vec3(1.0 - texture(mainTex, texCoord)), 1.0);
    }

    if(option == 2){
        fragColor = texture(mainTex, texCoord);
        float average = 0.2126 * fragColor.r + 0.7152 * fragColor.g + 0.0722 * fragColor.b;
        fragColor = vec4(average, average, average, 1.0);
    }

    if(option == 3){
        float kernel[9] = float[](
            -1, -1, -1,
            -1,  9, -1,
            -1, -1, -1
        );
        
        vec3 sampleTex[9];
        for(int i = 0; i < 9; i++){
            sampleTex[i] = vec3(texture(mainTex, texCoord.st + offsets[i]));
        }
        vec3 col = vec3(0.0);
        for(int i = 0; i < 9; i++)
            col += sampleTex[i] * kernel[i];
        
        fragColor = vec4(col, 1.0);
    }

    if(option == 4){
        float kernel[9] = float[](
            1.0 / 16, 2.0 / 16, 1.0 / 16,
            2.0 / 16, 4.0 / 16, 2.0 / 16,
            1.0 / 16, 2.0 / 16, 1.0 / 16 
        );
        
        vec3 sampleTex[9];
        for(int i = 0; i < 9; i++){
            sampleTex[i] = vec3(texture(mainTex, texCoord.st + offsets[i]));
        }
        vec3 col = vec3(0.0);
        for(int i = 0; i < 9; i++)
            col += sampleTex[i] * kernel[i];
        
        fragColor = vec4(col, 1.0);
    }
}
#endif