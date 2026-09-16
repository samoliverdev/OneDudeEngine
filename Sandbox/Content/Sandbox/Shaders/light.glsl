#pragma BeginPassDef
    Name MainPass
    RenderPass DefaultWindows TestPass
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl
#include Engine/ShaderLibrary/Vertex.glsl

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
    Out(3) vec3 worldPos;
    Out(4) vec3 worldNormal;

    void main() {
        mat4 _model = GetModelMatrix();
        vec4 _pos = GetLocalPos();

        worldPos = vec3(_model * _pos);
        //worldNormal = vec3(model * vec4(normal, 1.01));
        worldNormal = mat3(transpose(inverse(_model))) * normal; // for non-uniform scale objects

        OutPosition = projection * view * _model * _pos;
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
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