#version 330 core

#pragma SupportInstancing true
#pragma BlendMode Off

#pragma BeginProperties
Float useInstancing 0
Color4 color
Texture2D mainTex White
Color4 color2
#pragma EndProperties

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;
layout (location = 2) in vec3 _normal;

layout (location = 10) in mat4 _modelInstancing;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

uniform float useInstancing = 0;

out VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
    vec4 fragPosLightSpace;
} vsOut;

void main(){
    mat4 targetModelMatrix = (useInstancing >= 1.0 ? _modelInstancing : model);

    vsOut.pos = _pos;
    vsOut.normal = _normal;
    vsOut.texCoord = _texCoord;
    vsOut.worldPos = vec3(targetModelMatrix * vec4(_pos, 1.0));
    //worldNormal = vec3(model * vec4(normal, 1.01));
    vsOut.worldNormal = mat3(transpose(inverse(targetModelMatrix))) * _normal; // for non-uniform scale objects
    vsOut.fragPosLightSpace = lightSpaceMatrix * vec4(vsOut.worldPos, 1);

    gl_Position = projection * view * targetModelMatrix * vec4(_pos, 1.0);
}
#endif

#if defined(FRAGMENT)
uniform vec4 color = vec4(1, 1, 1, 1);
uniform vec4 color2 = vec4(1, 1, 1, 1);
uniform sampler2D mainTex;
uniform sampler2D shadowMap;

//Lights
uniform vec3 ambientLight = vec3(0.1, 0.1, 0.1);
uniform vec3 directionalLightDir = vec3(45, -125, 0.0);
uniform vec3 directionalLightColor = vec3(1, 1, 1);

struct PointLight{
    vec3 position;
    vec3 color;
    float radius;
};
#define NR_POINT_LIGHTS 4  
uniform PointLight pointLights[NR_POINT_LIGHTS];

in VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
    vec4 fragPosLightSpace;
} fsIn;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out int fragColor2;

vec3 CalcDirectionalLight(){
    vec3 norm = normalize(fsIn.worldNormal);
    vec3 lightDir = normalize(directionalLightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    return directionalLightColor * diff;
}

float CalcPointLightAttenuation(vec3 pos, vec3 lightPos, float lightRadius){
	//vec3 direction = lightPos - pos;
    //return pow(clamp(1.0 - length(direction) / lightRadius, 0.0, 1.0), 2.0);
    
    float dist = length(lightPos - pos);
    float att = clamp(1.0 - dist/lightRadius, 0.0, 1.0); att *= att;
    //float att = clamp(1.0 - dist*dist/(lightRadius*lightRadius), 0.0, 1.0); att *= att;
    return att;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos){
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // attenuation 
    float attenuation = CalcPointLightAttenuation(fragPos, light.position, light.radius);
    // combine results
    vec3 diffuse = light.color  * diff;
    diffuse  *= attenuation;
    return diffuse;
} 

float ShadowCalculation(vec4 fragPosLightSpace){
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float bias = max((0.05/32) * (1.0 - dot(fsIn.worldNormal, directionalLightDir)), (0.005/16));
    bias = 0.001/2;
    bias = 0;
    
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 
    shadow = 0;

    int sampleRadius = 2;
    vec2 pixelSize = 1.0 / textureSize(shadowMap, 0);
    for(int y = -sampleRadius; y <= sampleRadius; y++){
        for(int x = -sampleRadius; x <= sampleRadius; x++){
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * pixelSize).r;
            if(currentDepth > closestDepth + bias)
                shadow += 1;
        }
    }
    shadow /= pow((sampleRadius * 2 + 1), 2);

    if(projCoords.z > 1.0) shadow = 0.0;

    return shadow;
}

void main() {
    vec4 texColor = texture(mainTex, fsIn.texCoord);
    //if(texColor.a < 0.1) discard;

    vec3 norm = normalize(fsIn.worldNormal);
    //vec3 viewDir = normalize(viewPos - FragPos);

    vec4 objectColor = texColor * color * color2;

    float shadow = ShadowCalculation(fsIn.fragPosLightSpace); 

    vec3 lightResult = CalcDirectionalLight();
    lightResult *= (1 - shadow);

    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        lightResult += CalcPointLight(pointLights[i], norm, fsIn.worldPos); 

    vec3 ambient = ambientLight;
    vec3 result = (ambient + lightResult) * objectColor.rgb;

    //result = (ambient + (1.0 - shadow) * lightResult) * objectColor.rgb; 
    //result = objectColor.rgb * (lightResult * (1.0 - shadow) + ambient); 
    result = objectColor.rgb * (lightResult + ambient); 
    //result = vec3(1-shadow, 1-shadow, 1-shadow);
    fragColor = vec4(result, 1);
    //fragColor = objectColor;

    //float gamma = 2.2;
    //fragColor.rgb = pow(fragColor.rgb, vec3(1.0/gamma));

    fragColor2 = 50;
}
#endif