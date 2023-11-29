#pragma once

#include <OD/OD.h>
#include <OD/Loader/GLTFLoader.h>
#include <OD/AnimationSystem/RearrangeBones.h>
#include <ImGuizmo/ImGuizmo.h>
#include "CameraMovement.h"
#include "Ultis.h"
#include <OD/Loader/AssimpLoader.h>

using namespace OD;

struct AnimationInstance{
	Pose mAnimatedPose;
	std::vector<Matrix4> mPosePalette;
	unsigned int mClip = 0;
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

    //Character1
    std::vector<Ref<Mesh>> char1Meshs;
    Skeleton char1Skeleton;
    //std::vector<FastClip> clips;
    std::vector<Clip> char1Clips;
    AnimationInstance char1Anim;

    //Character2
    Ref<Model> char2Model;
    std::vector<Clip> char2Clips;
    AnimationInstance char2Anim;

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
        
        cgltf_data* char1 = OD::LoadGLTFFile("res/gltf/Woman.gltf");
        //cgltf_data* woman = OD::LoadGLTFFile("res/models/Soldier.glb");
        char1Meshs = OD::LoadMeshes(char1);
        char1Skeleton = LoadSkeleton(char1);

        //OD::BoneMap bm = OD::RearrangeSkeleton(skeleton);
        //OD::RearrangeMesh(*meshs[0], bm);
        
        auto _clips = OD::LoadAnimationClips(char1);
        for(auto i: _clips){
            char1Clips.push_back(i);
            //clips.push_back(OD::OptimizeClip(i));
            //OD::RearrangeFastclip(clips[clips.size()-1], bm);
        }

        OD::FreeGLTFFile(char1);

        char1Anim.mAnimatedPose = char1Skeleton.GetRestPose();
        char1Anim.mPosePalette.resize(char1Skeleton.GetRestPose().Size());
        char1Anim.mModel.localPosition(Vector3(0, 0, 0));
        //anim.mModel.localEulerAngles(Vector3(0, -90, 0));

        for(unsigned int i = 0; i < char1Clips.size(); ++i){
            if(char1Clips[i].GetName() == "Walk"){
                char1Anim.mClip = i;
            }
        }

        LogInfo("Clip Count: %zd", char1Clips.size());
        for(auto i: char1Clips){
            LogInfo("Clip Name: %s", i.GetName().c_str());
        }
        LogInfo("Mesh Count: %zd", char1Meshs.size());

        //char2Model = OD::AssimpLoadModel("res/animations/Walking.fbx", shader, &char2Clips);
        char2Model = OD::AssimpLoadModel("res/animations/Walking.dae", shader, &char2Clips);
        //char2Model = OD::AssimpLoadModel("res/gltf/Woman.gltf", shader, &char2Clips);
        
        char2Anim.mAnimatedPose = char2Model->skeleton.GetRestPose();
        char2Anim.mPosePalette.resize(char2Model->skeleton.GetRestPose().Size());
        char2Anim.mModel.localPosition(Vector3(3, 0, 0));
        //char2Anim.mModel.localScale(Vector3(0.05f, 0.05f, 0.05f));
        //char2Anim.mModel.localEulerAngles(Vector3(0, 180, 0));

        LogInfo("Char2 Clip Count: %zd", char2Clips.size());
        for(auto i: char2Clips){
            LogInfo("Char2 Clip Name: %s", i.GetName().c_str());
        }

    };

    void OnUpdate(float deltaTime) override {
        float char1T = char1Anim.mPlayback;
        float char2T = char2Anim.mPlayback;

        if(animate && Application::deltaTime() > 0){
            char1T += Application::deltaTime();  
            char2T += Application::deltaTime(); 
        }

        char1Anim.mPlayback = char1Clips[char1Anim.mClip].Sample(char1Anim.mAnimatedPose, char1T);
        char1Anim.mAnimatedPose.GetMatrixPalette(char1Anim.mPosePalette);

        char2Anim.mPlayback = char2Clips[char2Anim.mClip].Sample(char2Anim.mAnimatedPose, char2T);
        char2Anim.mAnimatedPose.GetMatrixPalette(char2Anim.mPosePalette, char2Model->skeleton.GetInvBindPose());
        //char2Model->skeleton.GetBindPose().GetMatrixPalette(char2Anim.mPosePalette);

        camMove.OnUpdate();
    };

    void OnRender(float deltaTime) override {
        cam.SetPerspective(60, 0.1f, 1000.0f, Application::screenWidth(), Application::screenHeight());
        cam.view = math::inverse(camTransform.GetLocalModelMatrix());

        Renderer::Begin();
        Renderer::Clean(0.1f, 0.1f, 0.1f, 1);
        Renderer::SetCamera(cam);

        std::vector<Matrix4>& invBindPose = char1Skeleton.GetInvBindPose();
        for(int i = 0; i < char1Anim.mPosePalette.size(); ++i){
            char1Anim.mPosePalette[i] = char1Anim.mPosePalette[i] * invBindPose[i];
        }

        shader->Bind();
        shader->SetMatrix4("animated", char1Anim.mPosePalette);
        shader->SetTexture2D("mainTex", *texture, 0);

        for(auto i: char1Meshs){
            Renderer::DrawMesh(*i, char1Anim.mModel.GetLocalModelMatrix(), *shader);
        }

        for(int i = 0; i < char1Anim.mAnimatedPose.Size(); i++){
            if(char1Anim.mAnimatedPose.GetParent(i) < 0) continue;
		
            Vector3 p0 = char1Anim.mAnimatedPose.GetGlobalTransform(i).localPosition();
            Vector3 p1 = char1Anim.mAnimatedPose.GetGlobalTransform(char1Anim.mAnimatedPose.GetParent(i)).localPosition();

            Renderer::DrawLine(p0, p1, Vector3(0, 1, 0), 1);
        }
        //*/

        /*std::vector<Matrix4>& invBindPose2 = char2Model->skeleton.GetInvBindPose();
        for(int i = 0; i < char2Anim.mPosePalette.size(); ++i){
            char2Anim.mPosePalette[i] = char2Anim.mPosePalette[i] * invBindPose2[i];
        }*/

        char2Model->materials[0]->UpdateUniforms();
        char2Model->materials[0]->shader()->Bind();
        char2Model->materials[0]->shader()->SetMatrix4("animated", char2Anim.mPosePalette);

        for(auto i: char2Model->meshs){
            Renderer::DrawMesh(
                *i, 
                char2Anim.mModel.GetLocalModelMatrix(), 
                *char2Model->materials[0]->shader()
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
        ImGui::SliderFloat("Char1_T", &char1Anim.mPlayback, 0, char1Clips[char1Anim.mClip].GetEndTime());
        ImGui::SliderFloat("Char2_T", &char2Anim.mPlayback, 0, char2Clips[char2Anim.mClip].GetEndTime());
        ImGui::End();

        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        cam.SetPerspective(60, 0.1f, 1000.0f, Application::screenWidth(), Application::screenHeight());
        cam.view = math::inverse(camTransform.GetLocalModelMatrix());

        if(Input::IsKeyDown(KeyCode::N)){
            curBone += 1;
            if(curBone >= char2Anim.mAnimatedPose.Size()){
                curBone = 0;
            }
        }

        Matrix4 trans = char2Anim.mAnimatedPose.GetLocalTransform(curBone).GetLocalModelMatrix();

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
            char2Anim.mAnimatedPose.SetLocalTransform(curBone, Transform(trans));
        }

    };

    void OnResize(int width, int height) override {};
};