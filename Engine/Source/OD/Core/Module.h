#pragma once
#include "OD/Defines.h"
#include <string>

namespace OD {

class OD_API Module {
public:
    virtual ~Module() = default;

    virtual void OnInit(){};
    virtual void OnExit(){};
    virtual void OnUpdate(float deltaTime){};
    virtual void OnRender(float deltaTime){};
    virtual void OnGUI(){};
    virtual void OnResize(int width, int height){};

    inline virtual bool DeleteOnExit(){ return true; }
    inline virtual int ExecutionSortPriority(){ return 1; };

    inline const std::string& Name(){ return name; }

protected:
    std::string name;
};  

typedef void (*_OnInit)();
typedef void (*_OnExit)();
typedef void (*_OnUpdate)(float);
typedef void (*_OnRender)(float);
typedef void (*_OnGUI)();
typedef void (*_OnResize)(int, int);

class OD_API FuncModule: public Module{
public:
    _OnInit onInit = nullptr;
    _OnExit onExit = nullptr;
    _OnUpdate onUpdate = nullptr;
    _OnRender onRender = nullptr;
    _OnGUI onGUI = nullptr;
    _OnResize onResize = nullptr;

    virtual void OnInit() override {if(onInit != nullptr) onInit(); }
    virtual void OnExit() override {if(onExit != nullptr) onExit(); }
    virtual void OnUpdate(float deltaTime) override {if(onUpdate != nullptr) onUpdate(deltaTime); }
    virtual void OnRender(float deltaTime) override {if(onRender != nullptr) onRender(deltaTime); }
    virtual void OnGUI() override {if(onGUI != nullptr) onGUI(); }
    virtual void OnResize(int width, int height) override {if(onResize != nullptr) onResize(width, height); }
};

}