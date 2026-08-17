#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Scene/Scene.h"
#include "RenderingPath.h"
#include "PassRenderSettings.h"

namespace OD{

class Framebuffer;

struct OD_API EnvironmentProbeComponent{
    float radius = 50.0f;
    int resolution = 512;
    float refreshRate = 0.0f;        // 0 = every frame, >0 = interval
    float lastUpdateTime = 0.0f;
    bool realtime = false;
    bool genMipmap = false;

    LayerMask cullingMask;

    RenderingPath renderingPath = RenderingPath::Forward;

    PassRenderSettings drawSettings = {true, true, false, false};
    PassCollectSettings collectSettings = {};

    Ref<Framebuffer> framebuffer = nullptr;

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDump(ar, CEREAL_NVP(radius));
        ArchiveDump(ar, CEREAL_NVP(resolution));
        ArchiveDump(ar, CEREAL_NVP(refreshRate));

        ArchiveDump(ar, CEREAL_NVP(genMipmap));

        ArchiveDump(ar, CEREAL_NVP(cullingMask));

        ArchiveDump(ar, CEREAL_NVP(renderingPath));

        ArchiveDump(ar, CEREAL_NVP(drawSettings));
        ArchiveDump(ar, CEREAL_NVP(collectSettings));
    }
};

}