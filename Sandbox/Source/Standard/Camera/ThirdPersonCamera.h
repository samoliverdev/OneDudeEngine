#pragma once
#include "OD/Scene/Scene.h"

using namespace OD;

namespace Standard{

class ThirdPersonCamera{
public:
    bool autoHiddenCursor = true;
    bool handleInputs = true;

    float upOffset = 1.4f;
    float rightOffset = 0.0f;
    float distance = 10.0f;
    float sensivity = 5;

    float yMin = -50.0f;
    float yMax = 50.0f;

    float currentX = 0.0f;
    float currentY = 0.0f;

    Entity lookAtTarget = EntityNull;

    double lastMousePosX = 0.0;
    double lastMousePosY = 0.0;

    float smoothMouseX = 0.0f;
    float smoothMouseY = 0.0f;
    float smoothingFactor = 25.0f; // Higher = more responsive
    
    bool hasStarted = false;
    
    void OnStart();
    void OnUpdate(Scene& scene, OD::TransformComponent& transform, bool updateInput = true);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, lookAtTarget);

        ArchiveDumpNVP(ar, autoHiddenCursor);
        ArchiveDumpNVP(ar, handleInputs);
        ArchiveDumpNVP(ar, upOffset);
        ArchiveDumpNVP(ar, rightOffset);
        ArchiveDumpNVP(ar, distance);
        ArchiveDumpNVP(ar, sensivity);
        ArchiveDumpNVP(ar, yMin);
        ArchiveDumpNVP(ar, yMax);

        ArchiveDumpNVP(ar, smoothingFactor);
    }
};

}