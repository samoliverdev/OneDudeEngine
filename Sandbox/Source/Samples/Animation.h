#pragma once
#include <OD/Core/Module.h>
#include <OD/Core/Transform.h>
#include <OD/Graphics/Camera.h>
#include <OD/Animation/Skeleton.h>
#include <OD/Animation/CrossFadeController.h>
#include "Ultis/CameraMovement.h"

namespace OD{
    class Shader;
    class Material;
    class Texture2D;
    class Mesh;
}

using namespace OD;

struct AnimationInstance{
	Pose mAnimatedPose;
	AlignedVector<Matrix4> mPosePalette;
	unsigned int mClip = 0;
	float mPlayback;
	Transform mModel;

	inline AnimationInstance(): mClip(0), mPlayback(0.0f){}
};

struct AnimationSample: public OD::Module{
    Ref<Shader> shader;
    Ref<Material> mat;
    Ref<Texture2D> texture;
    
    Transform camTransform;
    Camera cam;
    CameraMovement camMove;

    //Character1
    std::vector<Ref<Mesh>> char1Meshs;
    Skeleton char1Skeleton;
    //std::vector<FastClip> clips;
    std::vector<ClipT> char1Clips;
    AnimationInstance char1Anim;

    CrossFadeController char1Controller;

    //Character2
    Ref<Model> char2Model;
    Ref<Model> char2Animations;
    AnimationInstance char2Anim;
    CrossFadeController char2Controller;

    int curBone;
    bool animate = true;

    AnimationSample(){ name ="AnimationSample"; }
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;
    void OnExit() override;
};