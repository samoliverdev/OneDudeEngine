#pragma BeginProperties
    Color4 color
    Vector4 sizeOffset
    Texture2D mainTex White
    Texture2D normal Normal
    Float normalStrength 1 0 10
    Texture2D emissionMap Black
    Color4 emissionColor
    Texture2D maskMap White
    Float occlusion 1 0 1
    Float metallic 0 0 1
    Float smoothness 0.5 0.0 1.0
    Float cutoff 0.5 0 1
#pragma EndProperties

#pragma BeginPassDef
    Name MainPass
    CullFace BACK
    DepthTest LESS
    Blend Off
    SupportInstancing true
    MultiCompile _ SKINNED INSTANCING
    MultiCompile Opaque Blend
    MultiCompile Forward Deferred
#pragma EndPassDef

#pragma BeginPassDef
    Name ShadowmapPass
    CullFace BACK
    DepthTest LESS
    Blend Off
    SupportInstancing true
    MultiCompile _ SKINNED INSTANCING
#pragma EndPassDef
    
#include Engine/ShaderLibrary/Base.glsl

BeginPass(MainPass)
    BeginPassVertex
        void main(){
            
        }
    EndVertex

    BeginFragment
        void main(){

        }
    EndFragment
EndPass

BeginPassVertex(ShadowmapPass)
    void main(){

    }
EndPass