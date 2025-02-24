#pragma BeginPassDef
    Name MainPass
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(1, 0, PerDraw)
    Uniform mat4 model;
EndUniform()

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
EndUniform()

BeginUniform(0, 0, Main)
    Uniform vec3 material_ambient;
    Uniform vec3 material_diffuse;
    Uniform vec3 material_specular;
    Uniform float material_shininess;

    Uniform vec3 light_position;
    Uniform vec3 light_ambient;
    Uniform vec3 light_diffuse;
    Uniform vec3 light_specular;

    Uniform vec3 color;
    Uniform vec3 viewPos;
EndUniform()

#if defined(VERTEX) && defined(MainPass)
    Attribute(0) vec3 _pos;
    Attribute(1) vec2 _texCoord;
    Attribute(2) vec3 _normal;

    Out(0) vec3 pos;
    Out(1) vec3 normal;
    Out(2) vec2 texCoord;
    Out(3) vec3 worldPos;
    Out(4) vec3 worldNormal;

    void main() {
        pos = _pos;
        normal = _normal;
        texCoord = _texCoord;
        worldPos = vec3(model * vec4(pos, 1.0));
        //worldNormal = vec3(model * vec4(normal, 1.01));
        worldNormal = mat3(transpose(inverse(model))) * normal; // for non-uniform scale objects

        OutPosition = projection * view * model * vec4(pos, 1.0);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    In(0) vec3 pos;
    In(1) vec3 normal;
    In(2) vec2 texCoord;
    In(3) vec3 worldPos;
    In(4) vec3 worldNormal;

    Out(0) vec4 fragColor;

    void main() {
        vec3 objectColor = /*texture(texture1, texCoord).rgb **/ color;

        vec3 ambient = light_ambient * material_ambient;

        vec3 norm = normalize(worldNormal);
        vec3 lightDir = normalize(light_position - worldPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = light_diffuse * (diff * material_diffuse);

        vec3 viewDir = normalize(viewPos - worldPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
        vec3 specular = light_specular * (spec * material_specular);

        vec3 result = (ambient + diffuse + specular)/* * objectColor*/;
        fragColor = vec4(result, 1);
    }
#endif