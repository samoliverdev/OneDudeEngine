#pragma once

#include <OD/OD.h>
#include <OD/Loader/GLTFLoader.h>
#include "CameraMovement.h"
#include "Ultis.h"

using namespace OD;

struct AnimationInstance{
	Pose mAnimatedPose;
	std::vector<Matrix4> mPosePalette;
	unsigned int mClip;
	float mPlayback;
	Transform mModel;

	inline AnimationInstance() : mClip(0), mPlayback(0.0f) { }
};

struct Animation_7: public OD::Module{
    Ref<Shader> shader;
    Ref<Texture2D> texture;
    
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    std::vector<Ref<Mesh>> meshs;
    Pose pose;
    Skeleton skeleton;
    std::vector<Clip> clips;

    AnimationInstance anim;

    void OnInit() override {
        LogInfo("Game Init");

        Application::vsync(false);

        camTransform.localPosition(Vector3(0, 2, 4));
        camTransform.localEulerAngles(Vector3(-25, 0, 0));
        camMove.transform = & camTransform;
        camMove.OnStart();

        texture = AssetManager::Get().LoadTexture2D("res/gltf/Woman.png");
        shader = AssetManager::Get().LoadShaderFromFile("res/Builtins/Shaders/SkinnedModel.glsl");
        cgltf_data* woman = OD::LoadGLTFFile("res/gltf/Woman.gltf");

        meshs = OD::LoadMeshes(woman);
        pose = OD::LoadRestPose(woman);
        skeleton = LoadSkeleton(woman);
        clips = OD::LoadAnimationClips(woman);

        OD::FreeGLTFFile(woman);

        anim.mAnimatedPose = skeleton.GetRestPose();
        anim.mPosePalette.resize(skeleton.GetRestPose().Size());
        anim.mModel.localPosition(Vector3(0, 0, 0));
        //anim.mModel.localEulerAngles(Vector3(0, -90, 0));

        unsigned int numUIClips = (unsigned int)clips.size();
        for(unsigned int i = 0; i < numUIClips; ++i){
            if(clips[i].GetName() == "Walking"){
                anim.mClip = i;
            }
        }

        LogInfo("Clip Count: %zd", clips.size());
        for(auto i: clips){
            LogInfo("Clip Name: %s", i.GetName().c_str());
        }

        LogInfo("Mesh Count: %zd", meshs.size());
    };

    void OnUpdate(float deltaTime) override {
        anim.mPlayback = clips[anim.mClip].Sample(anim.mAnimatedPose, anim.mPlayback + Application::deltaTime());
        anim.mAnimatedPose.GetMatrixPalette(anim.mPosePalette);

        camMove.OnUpdate();
    };

    void OnRender(float deltaTime) override {
        cam.SetPerspective(60, 0.1f, 1000.0f, Application::screenWidth(), Application::screenHeight());
        cam.view = math::inverse(camTransform.GetLocalModelMatrix());

        Renderer::Begin();
        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);
        Renderer::SetCamera(cam);

        skeleton.GetBindPose().GetMatrixPalette(anim.mPosePalette);
        //pose.GetMatrixPalette(anim.mPosePalette);
        anim.mAnimatedPose.GetMatrixPalette(anim.mPosePalette);
        shader->Bind();
        shader->SetMatrix4("pose", anim.mPosePalette);
        shader->SetMatrix4("invBindPose", skeleton.GetInvBindPose());
        shader->SetTexture2D("mainTex", *texture, 0);

        for(auto i: meshs){
            Renderer::DrawMesh(*i, anim.mModel.GetLocalModelMatrix(), *shader);
        }

        for(int i = 0; i < anim.mAnimatedPose.Size(); i++){
            if(anim.mAnimatedPose.GetParent(i) < 0) continue;
		
            Vector3 p0 = anim.mAnimatedPose.GetGlobalTransform(i).localPosition();
            Vector3 p1 = anim.mAnimatedPose.GetGlobalTransform(anim.mAnimatedPose.GetParent(i)).localPosition();

            Renderer::DrawLine(p0, p1, Vector3(0, 1, 0), 1);
        }

        Renderer::End();
    };

    void OnGUI() override {};
    void OnResize(int width, int height) override {};
};