#pragma once

namespace OD{
    struct TransformComponent;
}

namespace Standard{

class FreeCamera{
public:
    float moveSpeed = 10;
    float rotSpeed = 50;

    float lastX = 400, lastY = 300;

    float yaw = 0;
    float pitch = 0;
    bool hasStarted = false;

    void OnStart(OD::TransformComponent& transform);
    void OnUpdate(OD::TransformComponent& transform);

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, moveSpeed);
        ArchiveDumpNVP(ar, rotSpeed);
    }
};

}