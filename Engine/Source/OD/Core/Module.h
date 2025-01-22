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

    inline virtual bool DeleteOnExit(){ return true; }
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