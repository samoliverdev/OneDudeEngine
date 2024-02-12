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
#pragma DrawType Instancing

#pragma CullFace BACK
#pragma DepthTest LESS
#pragma Blend Off

#ifdef VERTEX
/*
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
layout (location = 10) in mat4 modelInstancing;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float useInstancing = 0;
*/

#define INSTANCING
#include res/Engine/ShaderLibrary/Vertex.glsl

out VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
} vsOut;

void main(){
    //mat4 targetModelMatrix = (useInstancing >= 1.0 ? modelInstancing : model);

    vsOut.pos = pos;
    vsOut.normal = normal;
    vsOut.texCoord = texCoord;
    vsOut.worldPos = vec3(targetModelMatrix * vec4(pos, 1.0));
    //worldNormal = vec3(model * vec4(normal, 1.01));
    vsOut.worldNormal = mat3(transpose(inverse(targetModelMatrix))) * normal; // for non-uniform scale objects

    //gl_Position = projection * view * targetModelMatrix * vec4(pos, 1.0);
    gl_Position = projection * view * GetModelMatrix() * GetLocalPos();
}
#endif

#ifdef FRAGMENT

uniform mat4 view;

#include res/Engine/ShaderLibrary/Common.glsl
#include res/Engine/ShaderLibrary/Surface.glsl
#include res/Engine/ShaderLibrary/Shadows.glsl
#include res/Engine/ShaderLibrary/Light.glsl
#include res/Engine/ShaderLibrary/BRDF.glsl
#include res/Engine/ShaderLibrary/Lighting.glsl

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