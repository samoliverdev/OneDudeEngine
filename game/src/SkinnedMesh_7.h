#pragma once

#include <OD/OD.h>
#include <OD/Editor/Editor.h>
#include <OD/Renderer/Animations.h>
#include "CameraMovement.h"

using namespace OD;

struct SkinnedMesh_7: OD::Module {
    Ref<Model> model;
    Ref<Shader> shader;
    Transform modelTransform;
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    bool lastInput;
    bool input;

    int selectedBoneIndex;

    Ref<Model> danceAnimation;
    Animator* animator;
    int curAnimation = 0;

    Editor editor;

    void OnInit() override {
        LogInfo("Game Init");

        Application::vsync(false);

        modelTransform.localPosition(Vector3::zero);
        camTransform.localPosition(Vector3(0, 207, 235));
        camTransform.localEulerAngles(Vector3(-25, 0, 0));

        camMove.transform = &camTransform;
        camMove.moveSpeed = 60;
        camMove.OnStart();

        ///*
        model = AssetManager::Get().LoadModel(
            "res/models/Skinned/VampireALusth.dae",
            AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/SkinnedModel.glsl")
        );
        danceAnimation = AssetManager::Get().LoadModel("res/models/Skinned/Capoeira.dae");
        animator = new Animator(danceAnimation->animations[0].get());
        //*/

        /*
        model = AssetManager::GetGlobal()->LoadModel(
            "res/models/UnarmedWalkForward/UnarmedWalkForward.dae",
            GlobalAssetManager::LoadShaderFromFile("res/Builtins/Shaders/SkinnedModel.glsl")
        );
        animator = new Animator(model->animations[curAnimation].get());
        */

        LogInfo("Model Info -> meshs: %zd, materials: %zd, animations: %zd", model->meshs.size(), model->materials.size(), model->animations.size());
    }

    void OnUpdate(float deltaTime) override {
        OD_PROFILE_FUNCTION();

        camMove.OnUpdate();
        //modelTransform.localEulerAngles(Vector3(0, Platform::GetTime() * 20, 0));
        modelTransform.localScale(Vector3(50, 50, 50));

        lastInput = input;
        input = Input::IsKey(KeyCode::R);

        animator->UpdateAnimation(Application::deltaTime());

        auto transforms = animator->finalBoneMatrices();
        
        for (int i = 0; i < transforms.size(); ++i){
            std::string s = "finalBonesMatrices[" + std::to_string(i) + "]";
            for(auto j: model->materials){
                j->BindGlobal();
                j->SetGlobalMatrix4(s.c_str(), transforms[i]);
            }
        }

        if(input == true && lastInput == false){
            curAnimation += 1;
            if(curAnimation >= model->animations.size()) curAnimation = 0;

            animator->PlayAnimation(model->animations[curAnimation].get());
            /*
            selectedBoneIndex += 1;
            if(selectedBoneIndex >= model->boneCounter()){
                selectedBoneIndex = 0;
            }
            for(auto i: model->materials){
                i->shader->SetInt("selectedBoneIndex", selectedBoneIndex);
            }
            */
        }
    }   

    void OnRender(float deltaTime) override {
        OD_PROFILE_FUNCTION();

        cam.SetPerspective(60, 0.1f, 1000.0f, Application::screenWidth(), Application::screenHeight());
        cam.view = camTransform.GetLocalModelMatrix().inverse();

        Renderer::Begin();
        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);
        Renderer::SetCamera(cam);

        //Renderer::SetRenderMode(Renderer::RenderMode::WIREFRAME);
        Renderer::DrawModel(*model, modelTransform.GetLocalModelMatrix());

        Renderer::End();
    }

    void OnGUI() override {
        //static bool show;
        //ImGui::ShowDemoWindow(&show);

        //Vector3 camPos = camTransform.localPosition();
        //float translation[] = {camPos.x, camPos.y, camPos.z};
        //ImGui::SliderFloat3("position", translation, -5, 5);
    }

    void OnResize(int width, int height) override {}
};