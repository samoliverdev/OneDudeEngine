#pragma BeginPassDef
    Name DepthPass
    RenderPass DirectionalShadow OtherShadow
    SupportInstancing true
    DrawType _ INSTANCING
    CullFace BACK
    DepthTest LESS
    Blend Off
#pragma EndPassDef
#include Engine/Shaders/TerrainSystem/TerrainGeometry.glsl
#if defined(VERTEX)
void main(){ OutPosition=projection*view*TerrainPosition(TerrainUV()); }
#endif
#if defined(FRAGMENT)
void main(){}
#endif
