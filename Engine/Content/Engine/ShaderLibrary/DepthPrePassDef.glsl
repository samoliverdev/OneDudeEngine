#ifndef DEPTHPREPASS_INCLUDED
#define DEPTHPREPASS_INCLUDED

#pragma BeginPassDef
    Name DepthPrePass1
    CullFace BACK
    DepthTest LESS
    Blend Off
    SupportInstancing true
    MultiCompile _ SKINNED INSTANCING
    MultiCompile Forward Deferred
#pragma EndPassDef

#pragma BeginPassDef
    Name DepthPrePass2
    CullFace BACK
    DepthTest LESS
    Blend Off
    SupportInstancing true
    MultiCompile _ SKINNED INSTANCING
    MultiCompile Forward Deferred
#pragma EndPassDef

#endif
