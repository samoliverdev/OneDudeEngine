#pragma once

#include <OD/OD.h>
#include <OD/Loader/GLTFLoader.h>
#include <OD/AnimationSystem/RearrangeBones.h>
#include <ImGuizmo/ImGuizmo.h>
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
    //std::vector<FastClip> clips;
    std::vector<Clip> clips;

    AnimationInstance anim;

    Ref<Model> skinnedModel;

    int curBone;

    bool animate = true;

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
        //cgltf_data* woman = OD::LoadGLTFFile("res/models/Soldier.glb");

        skinnedModel = Model::CreateFromFile("res/models/Skinned/Ch02_nonPBR.dae", shader);

        meshs = OD::LoadMeshes(woman);
        pose = OD::LoadRestPose(woman);
        skeleton = LoadSkeleton(woman);

        //OD::BoneMap bm = OD::RearrangeSkeleton(skeleton);
        //OD::RearrangeMesh(*meshs[0], bm);
        
        auto _clips = OD::LoadAnimationClips(woman);
        for(auto i: _clips){
            clips.push_back(i);
            //clips.push_back(OD::OptimizeClip(i));
            //OD::RearrangeFastclip(clips[clips.size()-1], bm);
        }

        OD::FreeGLTFFile(woman);

        anim.mAnimatedPose = skeleton.GetRestPose();
        anim.mPosePalette.resize(skeleton.GetRestPose().Size());
        anim.mModel.localPosition(Vector3(0, 0, 0));
        //anim.mModel.localEulerAngles(Vector3(0, -90, 0));

        unsigned int numUIClips = (unsigned int)clips.size();
        for(unsigned int i = 0; i < numUIClips; ++i){
            if(clips[i].GetName() == "Walk"){
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
        float t = anim.mPlayback;
        if(animate && Application::deltaTime() > 0){
            t += Application::deltaTime();  
        }

        //anim.mPlayback = clips[anim.mClip].Sample(anim.mAnimatedPose, t);
        anim.mAnimatedPose.GetMatrixPalette(anim.mPosePalette);

        camMove.OnUpdate();
    };

    void OnRender(float deltaTime) override {
        cam.SetPerspective(60, 0.1f, 1000.0f, Application::screenWidth(), Application::screenHeight());
        cam.view = math::inverse(camTransform.GetLocalModelMatrix());

        Renderer::Begin();
        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);
        Renderer::SetCamera(cam);

        //skeleton.GetBindPose().GetMatrixPalette(anim.mPosePalette);
        //pose.GetMatrixPalette(anim.mPosePalette);
        //anim.mAnimatedPose.GetMatrixPalette(anim.mPosePalette);

        ///*
        std::vector<Matrix4>& invBindPose = skeleton.GetInvBindPose();
        for(int i = 0; i < anim.mPosePalette.size(); ++i){
            anim.mPosePalette[i] = anim.mPosePalette[i] * invBindPose[i];
        }

        shader->Bind();
        shader->SetMatrix4("animated", anim.mPosePalette);
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
        //*/

        
        ///*
        std::vector<Matrix4> mPosePalette2;
        skinnedModel->skeleton.GetBindPose().GetMatrixPalette(mPosePalette2);
        std::vector<Matrix4>& invBindPose2 = skinnedModel->skeleton.GetInvBindPose();
        for(int i = 0; i < mPosePalette2.size(); ++i){
            mPosePalette2[i] = mPosePalette2[i] * invBindPose2[i];
        }

        skinnedModel->materials[0]->UpdateUniforms();
        skinnedModel->materials[0]->shader()->Bind();
        skinnedModel->materials[0]->shader()->SetMatrix4("animated", mPosePalette2);

        for(auto i: skinnedModel->meshs){
            Renderer::DrawMesh(
                *i, 
                Mathf::TRS(Vector3(5, 0, 0), QuaternionIdentity, Vector3(0.05f, 0.05f, 0.05f)), 
                *skinnedModel->materials[0]->shader()
            );
        }
        //*/

        Renderer::End();
    };

    void OnGUI() override {
        ImGuizmo::Enable(true);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();

        ImGui::Begin("Animation");
        ImGui::Checkbox("Animated", &animate);
        ImGui::SliderFloat("T", &anim.mPlayback, 0, clips[anim.mClip].GetEndTime());
        ImGui::End();

        //ImGuizmo::SetDrawlist();
        //ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
        //ImGuizmo::SetRect(0, 0, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        cam.SetPerspective(60, 0.1f, 1000.0f, Application::screenWidth(), Application::screenHeight());
        cam.view = math::inverse(camTransform.GetLocalModelMatrix());

        if(Input::IsKeyDown(KeyCode::N)){
            curBone += 1;
            if(curBone >= skinnedModel->skeleton.GetBindPose().Size()){
                curBone = 0;
            }
        }

        //Matrix4 trans = anim.mAnimatedPose.GetLocalTransform(curBone).GetLocalModelMatrix();
        Matrix4 trans = skinnedModel->skeleton.GetBindPose().GetLocalTransform(curBone).GetLocalModelMatrix();

        bool snap = Input::IsKey(KeyCode::Control);
        float snapValue = 45;
        float snapValues[3] = {snapValue, snapValue, snapValue};
        ImGuizmo::OPERATION gizmoType = ImGuizmo::OPERATION::ROTATE;

        //Matrix4 _m = anim.mModel.GetLocalModelMatrix();
        //Matrix4 global = _m * trans;
        ImGuizmo::Manipulate(
            Mathf::Raw(cam.view),
            Mathf::Raw(cam.projection),
            gizmoType, 
            ImGuizmo::LOCAL,
            Mathf::Raw(trans),
            nullptr,
            (snap ? snapValues : nullptr)
        );

        if(ImGuizmo::IsUsing()){
            /*
            glm::vec3 s;
            glm::quat r;
            glm::vec3 t;
            glm::vec3 sk;
            glm::vec4 p;
            glm::decompose((glm::mat4)trans, s, r, t, sk, p);

            if(_gizmoType == Editor::GizmosType::Translation) tc.position(t);
            if(_gizmoType == Editor::GizmosType::Rotation) tc.rotation(r);
            if(_gizmoType == Editor::GizmosType::Scale) tc.localScale(s);
            */
            //gismosTarget = Transform(trans);
            //Matrix4 f = math::inverse(_m) * global;
            //anim.mAnimatedPose.SetLocalTransform(curBone, Transform(trans));
            skinnedModel->skeleton.GetBindPose().SetLocalTransform(curBone, Transform(trans));
        }

    };

    void OnResize(int width, int height) override {};
};