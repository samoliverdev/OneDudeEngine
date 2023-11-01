#pragma once

#include <OD/OD.h>
#include <OD/Loader/GLTFLoader.h>
#include "CameraMovement.h"
#include "Ultis.h"

using namespace OD;

struct Animation_7: public OD::Module {
    Pose pose;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    void OnInit() override {
        LogInfo("Game Init");

        Application::vsync(false);

        camTransform.localPosition(Vector3(0, 2, 4));
        camTransform.localEulerAngles(Vector3(-25, 0, 0));
        camMove.transform = & camTransform;
        camMove.OnStart();

        cgltf_data* woman = OD::LoadGLTFFile("res/gltf/Woman.gltf");

        pose = OD::LoadRestPose(woman);
        std::vector<Clip> clips =  OD::LoadAnimationClips(woman);

        LogInfo("Clip Count: %zd", clips.size());
        for(auto i: clips){
            LogInfo("Clip Name: %s", i.GetName().c_str());
        }
    };

    void OnUpdate(float deltaTime) override {
        camMove.OnUpdate();
    };

    void OnRender(float deltaTime) override {
        cam.SetPerspective(60, 0.1f, 1000.0f, Application::screenWidth(), Application::screenHeight());
        cam.view = math::inverse(camTransform.GetLocalModelMatrix());

        Renderer::Begin();
        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);
        Renderer::SetCamera(cam);

        for(int i = 0; i < pose.Size(); i++){
            if(pose.GetParent(i) < 0) continue;
		
            Vector3 p0 = pose.GetGlobalTransform(i).localPosition();
            Vector3 p1 = pose.GetGlobalTransform(pose.GetParent(i)).localPosition();

            Renderer::DrawLine(p0, p1, Vector3(0, 1, 0), 1);
        }

        Renderer::End();
    };

    void OnGUI() override {};
    void OnResize(int width, int height) override {};
};