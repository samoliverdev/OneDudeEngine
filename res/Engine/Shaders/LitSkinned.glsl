#version 330 core

#pragma BeginProperties
    Texture2D mainTex White
    Texture2D normal White
    Color4 color
    Float metallic 0 0 1
    Float smoothness 0.5 0.0 1.0
    Float cutoff 0.5 0 1
#pragma EndProperties

#pragma BeginPass 

#pragma SupportInstancing true
#pragma DrawType Skinned

#pragma CullFace BACK
#pragma DepthTest LESS
#pragma Blend Off

#pragma MultiCompile Default Skinned Instancing
#pragma MultiCompile Opaque Fade
#pragma MultiCompile Black White
/*
Default
    -Opaque
        -Black
Default
    -Opaque
        -White
Default
    -Fade
        -Black
Default
    -Fade
        -White

Skinned
    -Opaque
        -Black
Skinned
    -Opaque
        -White
Skinned
    -Fade
        -Black
Skinned
    -Fade
        -White

Instancing
    -Opaque
        -Black
Instancing
    -Opaque
        -White
Instancing
    -Fade
        -Black
Instancing
    -Fade
        -White
*/

#ifdef VERTEX

#define SKINNED
#include ../ShaderLibrary/Vertex.glsl

out VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
} vsOut;

void main(){
    //mat4 targetModelMatrix = (useInstancing >= 1.0 ? modelInstancing : model);
    mat4 targetModelMatrix = GetModelMatrix();

    vsOut.pos = pos;
    vsOut.normal = normal;
    vsOut.texCoord = texCoord;
    vsOut.worldPos = vec3(targetModelMatrix * vec4(pos, 1.0));
    //worldNormal = vec3(model * vec4(normal, 1.01));
    vsOut.worldNormal = mat3(transpose(inverse(targetModelMatrix))) * normal; // for non-uniform scale objects

    //gl_Position = projection * view * targetModelMatrix * vec4(pos, 1.0);
    gl_Position = projection * view * targetModelMatrix * GetLocalPos();
}
#endif

#ifdef FRAGMENT

uniform mat4 view;

#include ../ShaderLibrary/Common.glsl
#include ../ShaderLibrary/Surface.glsl
#include ../ShaderLibrary/Shadows.glsl
#include ../ShaderLibrary/Light.glsl
#include ../ShaderLibrary/BRDF.glsl
#include ../ShaderLibrary/Lighting.glsl

in VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
} fsIn;

uniform vec3 viewPos;

uniform sampler2D mainTex;
uniform vec4 color = vec4(1,1,1,1);
uniform float metallic = 0;
uniform float smoothness = 0.5;
uniform float cutoff  = 0.5;

out vec4 fragColor;

void main(){
    vec4 base = texture(mainTex, fsIn.texCoord);
    base = base * color;

    Surface surface;
    surface.position = fsIn.worldPos;
    surface.normal = normalize(fsIn.worldNormal);
    surface.viewDirection = normalize(viewPos - fsIn.worldPos);
    surface.depth = -(view * vec4(fsIn.worldPos, 1)).z;
    surface.color = base.rgb;
    surface.alpha = base.a;
    surface.metallic = metallic;
    surface.smoothness = smoothness;
    
    BRDF brdf = GetBRDF(surface);
    vec3 color = GetLightingFinal2(surface, brdf);
    fragColor = vec4(color, surface.alpha);

    if(base.a < cutoff) discard;
}
#endif

#pragma EndPass 