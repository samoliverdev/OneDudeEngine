#pragma once
#include "OD/Defines.h"

namespace OD {

class OD_API Module {
public:
    virtual void OnInit() = 0;
    virtual void OnExit() = 0;
    virtual void OnUpdate(float deltaTime) = 0;
    virtual void OnRender(float deltaTime) = 0;
    virtual void OnGUI() = 0;
    virtual void OnResize(int width, int height) = 0;
};  

}