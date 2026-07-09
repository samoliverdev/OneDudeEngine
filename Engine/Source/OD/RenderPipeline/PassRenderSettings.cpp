#include "PassRenderSettings.h"
#include "RenderContext.h"

namespace OD{

void PassCollectSettings::BuildMask(){
    rejectIfAny = 0;

    if(!collectMesh)         rejectIfAny |= RenderData::Flag::FromMesh;
    if(!collectModel)        rejectIfAny |= RenderData::Flag::FromModel;
    if(!collectSkinnedMesh)  rejectIfAny |= RenderData::Flag::FromSkinnedMesh;
    if(!collectSkinnedModel) rejectIfAny |= RenderData::Flag::FromSkinnedModel;
    if(!collectCluster)      rejectIfAny |= RenderData::Flag::FromCluster;
    if(!collectParticle)     rejectIfAny |= RenderData::Flag::IsParticle;
    if(!collectDecal)        rejectIfAny |= RenderData::Flag::IsDecal;
}

}