#version 410 core

#pragma SupportInstancing true
#pragma BlendMode Off

#pragma BeginProperties
Texture2D mainTex White
Texture2D normal White
Color4 color
Float shininess 0
#pragma EndProperties

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;
layout (location = 2) in vec3 _normal;
layout (location = 10) in mat4 _modelInstancing;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float useInstancing = 0;

out VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
} vsOut;

void main(){
    mat4 targetModelMatrix = (useInstancing >= 1.0 ? _modelInstancing : model);

    vsOut.pos = _pos;
    vsOut.normal = _normal;
    vsOut.texCoord = _texCoord;
    vsOut.worldPos = vec3(targetModelMatrix * vec4(_pos, 1.0));
    //worldNormal = vec3(model * vec4(normal, 1.01));
    vsOut.worldNormal = mat3(transpose(inverse(targetModelMatrix))) * _normal; // for non-uniform scale objects

    gl_Position = projection * view * targetModelMatrix * vec4(_pos, 1.0);
}
#endif

#if defined(FRAGMENT)
uniform vec4 color = vec4(1, 1, 1, 1);
uniform sampler2D mainTex;

uniform float shininess = 0;
uniform vec3 viewPos;

uniform mat4 view;

//////////////

uniform float shadowBias = 0.001;

//uniform sampler2D shadowMap;
//uniform mat4 lightSpaceMatrix;

uniform sampler2DArray cascadeShadowMapsA;

#define MAX_SHADOW_CASCADES 4
uniform sampler2D cascadeShadowMaps[MAX_SHADOW_CASCADES];
uniform mat4 cascadeShadowMatrixs[MAX_SHADOW_CASCADES];
uniform float cascadeShadowSplitDistances[MAX_SHADOW_CASCADES];
uniform int cascadeShadowCount = 0;

#define MAX_SPOTLIGHT_SHADOWS 5
uniform sampler2D spotlightShadowMaps[MAX_SPOTLIGHT_SHADOWS];
uniform mat4 spotlightSpaceMatrixs[MAX_SPOTLIGHT_SHADOWS];
uniform int spotlightShadowCount = 0;

//Lights
uniform vec3 ambientLight = vec3(0.1, 0.1, 0.1);
uniform vec3 directionalLightDir = vec3(45, -125, 0.0);
uniform vec3 directionalLightColor = vec3(1, 1, 1);
uniform float directionalLightspecular = 1;

struct Light{
    float type;
    vec3 pos;
    vec3 dir;

    vec3 color;
    float specular;
    float falloff;

    float radius;

    float coneAngleOuter;
    float coneAngleInner;
};
#define MAX_LIGHTS 12
uniform Light lights[MAX_LIGHTS];
uniform int lightsCount = 0;

in VsOut{
    vec3 pos;
    vec3 normal;
    vec2 texCoord;
    vec3 worldPos;
    vec3 worldNormal;
} fsIn;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out int fragColor2;

vec3 CalcDirectionalLight(){
    vec3 lightDir = normalize(directionalLightDir);
    vec3 viewDir = normalize(viewPos - fsIn.worldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    vec3 normal = normalize(fsIn.worldNormal);

    float attenuation = 1;

    float spec = pow(max(dot(normal, halfDir), 0.0), shininess) * directionalLightspecular;
    vec3 specular = directionalLightColor * spec;
    specular *= attenuation;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = directionalLightColor * diff;
    diffuse *= attenuation;

    return diffuse + specular;
}

float CalcPointLightAttenuation(vec3 pos, vec3 lightPos, float lightRadius){
	//vec3 direction = lightPos - pos;
    //return pow(clamp(1.0 - length(direction) / lightRadius, 0.0, 1.0), 2.0);
    
    float dist = length(lightPos - pos);
    float att = clamp(1.0 - dist/lightRadius, 0.0, 1.0); att *= att;
    //float att = clamp(1.0 - dist*dist/(lightRadius*lightRadius), 0.0, 1.0); att *= att;
    return att;
}

vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos){
    vec3 lightDir   = normalize(light.pos - fragPos);
    vec3 viewDir    = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float attenuation = CalcPointLightAttenuation(fragPos, light.pos, light.radius);

    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess) * light.specular;
    vec3 specular = light.color * spec;
    specular *= attenuation;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * diff;
    diffuse *= attenuation;

    return diffuse + specular;
} 

vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, out float att){
    vec3 lightDir = normalize(light.pos - fragPos);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    
    float attenuation = CalcPointLightAttenuation(fragPos, light.pos, light.radius);

    float theta = dot(-lightDir, normalize(light.dir));
    float epsilon = light.coneAngleInner - light.coneAngleOuter;
    attenuation *= clamp((theta - light.coneAngleOuter) / epsilon, 0.0, 1.0);  
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color  * diff;
    diffuse *= attenuation;

    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess) * light.specular;
    vec3 specular = light.color * spec;
    specular *= attenuation;

    att = attenuation; 

    return diffuse + specular;
} 

float ShadowCalculation(sampler2D shadowMap, vec4 fragPosLightSpace){
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    if(currentDepth > 1.0) return 0;

    // check whether current frag pos is in shadow
    float bias = max((0.05/32) * (1.0 - dot(normalize(fsIn.worldNormal), directionalLightDir)), (0.005/16));
    bias = shadowBias;
    //bias = 0;
    
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 
    
    shadow = 0;
    int sampleRadius = 2;
    vec2 pixelSize = 1.0 / textureSize(shadowMap, 0);
    for(int y = -sampleRadius; y <= sampleRadius; y++){
        for(int x = -sampleRadius; x <= sampleRadius; x++){
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * pixelSize).r;
            if(currentDepth > closestDepth + bias){
                shadow += 1;
            }
        }
    }
    shadow /= pow((sampleRadius * 2 + 1), 2);

    return shadow;
}

float ShadowCalculationCascade(in sampler2D _shadowMap, vec4 fragPosLightSpace, int cascadeIndex){
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(_shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    if(currentDepth > 1.0) return 0;

    // check whether current frag pos is in shadow
    float bias = max((0.05/32) * (1.0 - dot(normalize(fsIn.worldNormal), directionalLightDir)), (0.005/32));
    bias = shadowBias;
    //const float biasModifier = 0.5f;
    //bias *= 1 / cascadeShadowSplitDistances[cascadeShadowCount] * biasModifier;
    
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 
    
    shadow = 0;
    int sampleRadius = 2;
    vec2 pixelSize = 1.0 / vec2(textureSize(_shadowMap, 0));
    for(int x = -sampleRadius; x <= sampleRadius; x++){
        for(int y = -sampleRadius; y <= sampleRadius; y++){
            float closestDepth = texture(_shadowMap, projCoords.xy + vec2(x, y) * pixelSize).r;
            shadow += (currentDepth - bias) > closestDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= pow((sampleRadius * 2 + 1), 2);
    //shadow /= 9.0;

    if(projCoords.z > 1.0){
        shadow = 0.0;
    }

    return shadow;
}

float ShadowCalculationCascade2(vec4 fragPosLightSpace, int cascadeIndex){
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(cascadeShadowMapsA, vec3(projCoords.xy, cascadeIndex)).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    if(currentDepth > 1.0) return 0;

    // check whether current frag pos is in shadow
    float bias = max((0.05/32) * (1.0 - dot(normalize(fsIn.worldNormal), directionalLightDir)), (0.005/32));
    bias = shadowBias;
    //const float biasModifier = 0.5f;
    //bias *= 1 / cascadeShadowSplitDistances[cascadeShadowCount] * biasModifier;
    
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 
    
    shadow = 0;
    int sampleRadius = 2;
    vec2 pixelSize = 1.0 / vec2(textureSize(cascadeShadowMapsA, 0));
    for(int x = -sampleRadius; x <= sampleRadius; x++){
        for(int y = -sampleRadius; y <= sampleRadius; y++){
            float closestDepth = texture(cascadeShadowMapsA, vec3(projCoords.xy + vec2(x, y) * pixelSize, cascadeIndex)).r;
            shadow += (currentDepth - bias) > closestDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= pow((sampleRadius * 2 + 1), 2);
    //shadow /= 9.0;
    
    return shadow;
}

void main() {
    vec4 texColor = texture(mainTex, fsIn.texCoord);
    //if(texColor.a < 0.1) discard;

    vec3 norm = normalize(fsIn.worldNormal);
    //vec3 viewDir = normalize(viewPos - FragPos);

    vec4 objectColor = texColor * color;

    //float shadow = ShadowCalculation(shadowMap, lightSpaceMatrix * vec4(fsIn.worldPos, 1)); 

    float shadow = 0;

    vec4 fragPosViewSpace = view * vec4(fsIn.worldPos, 1);
    float depthValue = abs(fragPosViewSpace.z);
    //float depthValue = (projection * view * model * vec4(fsIn.pos, 1)).z;

    /*int cascadeIndex = 0;
    for(int i = 0; i < cascadeShadowCount; i++){
        if(depthValue < cascadeShadowSplitDistances[i]){
            cascadeIndex = i;
            break;
        }
    }
    shadow += ShadowCalculationCascade(cascadeShadowMaps[cascadeIndex], cascadeShadowMatrixs[cascadeIndex] * vec4(fsIn.worldPos, 1), cascadeIndex);*/ 

    int cascadeIndex = 0;
    for(int i = 0; i < cascadeShadowCount; i++){
        if(depthValue <= cascadeShadowSplitDistances[i]){
            cascadeIndex = i;
            //shadow = ShadowCalculationCascade(cascadeShadowMaps[i], cascadeShadowMatrixs[i] * vec4(fsIn.worldPos, 1), i);
            shadow = ShadowCalculationCascade2(cascadeShadowMatrixs[i] * vec4(fsIn.worldPos, 1), i);
            break;
        }
    }

    //for(int i = 0; i < spotlightShadowCount; i++){
    //    shadow += ShadowCalculation(spotlightShadowMaps[i], spotlightSpaceMatrixs[i] * vec4(fsIn.worldPos, 1)); 
    //}

    vec3 lightResult = CalcDirectionalLight();
    //lightResult *= (1 - shadow);

    for(int i = 0; i < lightsCount; i++){
        if(lights[i].type <= 1){
            lightResult += CalcPointLight(lights[i], norm, fsIn.worldPos); 
        } else if(lights[i].type <= 2){
            float att = 0;
            lightResult += CalcSpotLight(lights[i], norm, fsIn.worldPos, att); 
            //if(i < spotlightShadowCount){
            //    shadow += ShadowCalculation(spotlightShadowMaps[i], spotlightSpaceMatrixs[i] * vec4(fsIn.worldPos, 1)) * clamp(att, 0, 1); 
            //}
        }
    }

    lightResult *= (1 - shadow);

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

    int DEBUG_SHADOWS = 0;
    if(DEBUG_SHADOWS == 1){
        switch(cascadeIndex){
            case 0:
            fragColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
            break;
            case 1:
            fragColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
            break;
            case 2:
            fragColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
            break;
            default :
            fragColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
            break;
        }
    }

    fragColor2 = 50;
}
#endif