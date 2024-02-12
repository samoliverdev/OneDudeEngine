#version 330 core

#if defined(VERTEX)
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec2 _texCoord;
layout (location = 2) in vec3 _normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 pos;
out vec3 normal;
out vec2 texCoord;
out vec3 worldPos;
out vec3 worldNormal;

void main() {
    pos = _pos;
    normal = _normal;
    texCoord = _texCoord;
    worldPos = vec3(model * vec4(pos, 1.0));
    //worldNormal = vec3(model * vec4(normal, 1.01));
    worldNormal = mat3(transpose(inverse(model))) * normal; // for non-uniform scale objects

    gl_Position = projection * view * model * vec4(pos, 1.0);
}
#endif

#if defined(FRAGMENT)
struct Material{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform Material material;

struct Light {
    vec3 position;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Light light; 

uniform sampler2D texture1;
uniform vec3 color;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

in vec3 pos;
in vec3 normal;
in vec2 texCoord;
in vec3 worldPos;
in vec3 worldNormal;

out vec4 fragColor;

void main() {
    vec3 objectColor = texture(texture1, texCoord).rgb * color;

    vec3 ambient = light.ambient * material.ambient;

    vec3 norm = normalize(worldNormal);
    vec3 lightDir = normalize(light.position - worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);

    vec3 viewDir = normalize(viewPos - worldPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    vec3 result = (ambient + diffuse + specular)/* * objectColor*/;
    fragColor = vec4(result, 1);
}
#endif