#include "RendererFeature.h"

namespace OD{

RendererFeatureGlobal& RendererFeatureGlobal::Get(){
    static RendererFeatureGlobal instance;
    return instance;
}

}