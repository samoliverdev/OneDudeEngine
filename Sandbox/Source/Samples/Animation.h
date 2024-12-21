#pragma once
#include <OD/OD.h>
#include "Ultis/CameraMovement.h"

using namespace OD;

struct AnimationInstance{
	Pose mAnimatedPose;
	std::vector<Matrix4> mPosePalette;
	unsigned int mClip = 0;
	float mPlayback;
	Transform mModel;

	inline AnimationInstance(): mClip(0), mPlayback(0.0f){}
};

struct AnimationSample: public OD::Module{
    Ref<Shader> shader;
    Ref<Texture2D> texture;
    
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    //Character1
    std::vector<Ref<Mesh>> char1Meshs;
    Skeleton char1Skeleton;
    //std::vector<FastClip> clips;
    std::vector<Clip> char1Clips;
    AnimationInstance char1Anim;

    CrossFadeController char1Controller;

    //Character2
    Ref<Model> char2Model;
    Ref<Model> char2Animations;
    AnimationInstance char2Anim;
    CrossFadeController char2Controller;

    int curBone;
    bool animate = true;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};