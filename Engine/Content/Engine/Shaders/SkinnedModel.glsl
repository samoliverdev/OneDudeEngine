#pragma BeginProperties
    Color4 color
    Texture2D mainTex White
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    SupportInstancing true
    MultiCompile _ SKINNED
#pragma EndPassDef

#include Engine/ShaderLibrary/Base.glsl

BeginUniform(0, 0, Main)
    Uniform vec4 color;
    Uniform int selectedBoneIndex;
EndUniform()

BeginUniform(2, 0, CamDraw)
    Uniform mat4 projection;
    Uniform mat4 view;
EndUniform()

const int MAX_BONES = 120;
const int MAX_BONE_INFLUENCE = 4;

BeginUniform(1, 0, PerDraw)
    Uniform mat4 model;
EndUniform()

#if defined(SKINNED)
BeginUniform(1, 1, PerDrawSkinned)
    Uniform mat4 animated[MAX_BONES];
EndUniform()
#endif

#if defined(VERTEX) && defined(MainPass)
    Attribute(0) vec3 _pos;
    Attribute(1) vec2 _texCoord;
    Attribute(2) vec3 _normal;

    #if defined(SKINNED)
    Attribute(5) ivec4 _boneIds;
    Attribute(6) vec4 _weights;
    #endif

    Out(0) vec3 pos;
    Out(1) vec3 normal;
    Out(2) vec2 texCoord;

    #if defined(SKINNED)
    OutFlat(3) ivec4 boneIds;
    Out(4) vec4 weights;
    #endif

    void main(){
        pos = _pos;
        normal = _normal;
        texCoord = _texCoord;
        #if defined(SKINNED)
        boneIds = _boneIds;
        weights = _weights;
        #endif

        #if defined(SKINNED)
        mat4 skin = animated[boneIds.x] * weights.x +
        animated[boneIds.y] * weights.y +
        animated[boneIds.z] * weights.z +
        animated[boneIds.w] * weights.w;
        #else
        mat4 skin = mat4(1.0);
        #endif

        OutPosition = projection * view * model * skin * vec4(pos, 1.0);
    }
#endif

#if defined(FRAGMENT) && defined(MainPass)
    //uniform sampler2D mainTex;
    //uniform vec4 color = vec4(1,1,1,1);

    In(0) vec3 pos;
    In(1) vec3 normal;
    In(2) vec2 texCoord;
    #if defined(SKINNED)
    InFlat(3) ivec4 boneIds;
    In(4) vec4 weights;
    #endif

    Out(0) vec4 fragColor;

    float near = 0.1; 
    float far  = 100.0; 
    
    float LinearizeDepth(float depth){
        float z = depth * 2.0 - 1.0; // back to NDC 
        return (2.0 * near * far) / (far + near - z * (far - near));	
    }

    void main() {
        vec4 outColor = /*texture(mainTex, texCoord) **/ color;
        if(outColor.a < 0.1) discard;

        fragColor = outColor;
        //fragColor = color;
        return;

        //outColor = vec4(normal, 1);
        //outColor = vec4(texCoord, 0, 1);

        //fragColor = outColor;
        //fragColor = weights;
        //return;

        //fragColor = vec4(boneIds.x, boneIds.x, boneIds.x,1);

        //fragColor = weights;

        #if defined(SKINNED)
        for(int i = 0; i < 4; i++){
            if(boneIds[i] == selectedBoneIndex){
                if(weights[i] >= 0.7){
                    fragColor = vec4(1,0,0,0) * weights[i];
                } else if(weights[i] >= 0.4 && weights[i] <= 0.6){
                    fragColor = vec4(0,1,0,0) * weights[i];
                } else if(weights[i] >= 0.1){
                    fragColor = vec4(1,1,0,0) * weights[i];
                }
                break;
            }
        }
        #endif

        //float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
        //fragColor = vec4(vec3(depth), 1.0);
    }
#endif