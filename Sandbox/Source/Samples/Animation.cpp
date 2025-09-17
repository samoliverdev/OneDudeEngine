#include "Animation.h"
#include "Ultis/Ultis.h"
#include <OD/Loader/GLTFLoader.h>
#include <OD/Loader/AssimpLoader.h>
#include <OD/Animation/RearrangeBones.h>
#include <OD/Animation/CrossFadeController.h>
#include <OD/Core/Input.h>
#include <OD/Core/Application.h>
#include <OD/Graphics/Texture.h>
#include <OD/Graphics/Shader.h>
#include <OD/Graphics/Graphics.h>

void AnimationSample::OnInit(){
    LogInfo("Game Init");

    //Application::Vsync(false);

    camTransform.LocalPosition(Vector3(0, 2, 4));
    camTransform.LocalEulerAngles(Vector3(-25, 0, 0));
    camMove.transform = & camTransform;
    camMove.OnStart();

    texture = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Models/gltf/Woman.png");
    shader = AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/SkinnedModel.glsl");
    mat = CreateRef<Material>(shader);
    
    cgltf_data* char1 = OD::LoadGLTFFile("Sandbox/Models/gltf/Woman.gltf");
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
    char1Anim.mModel.LocalPosition(Vector3(0, 0, 0));
    //anim.mModel.localEulerAngles(Vector3(0, -90, 0));

    for(unsigned int i = 0; i < char1Clips.size(); ++i){
        if(char1Clips[i].GetName() == "Walk"){
            char1Anim.mClip = i;
        }
    }

    ClipT& _clip = char1Clips[char1Anim.mClip];
    for(int i = 0; i < _clip.Size(); i++){
        LogInfo("Clip Pos Frames: %d", _clip[i].GetPositionTrack().Size());
        LogInfo("Clip Rot Frames: %d", _clip[i].GetRotationTrack().Size());
        LogInfo("Clip Scale Frames: %d", _clip[i].GetScaleTrack().Size());
    }

    char1Controller.SetSkeleton(char1Skeleton);
    char1Controller.Play(&char1Clips[char1Anim.mClip]);
    char1Controller.Update(0.0f);
    char1Controller.GetCurrentPose().GetMatrixPalette(char1Anim.mPosePalette);

    LogInfo("Clip Count: %zd", char1Clips.size());
    for(auto i: char1Clips){
        LogInfo("Clip Name: %s", i.GetName().c_str());
    }
    LogInfo("Mesh Count: %zd", char1Meshs.size());

    //char2Model = OD::AssimpLoadModel("res/animations/Walking.fbx", shader);
    char2Model = CreateRef<Model>();
    #ifdef USE_ASSIMP
    OD::AssimpLoadModel(*char2Model, "Sandbox/Animations/Walking.dae", {shader});
    #else
    Assert(false && "Assimp Not Supported");
    #endif

    char2Animations = CreateRef<Model>();
    #ifdef USE_ASSIMP
    OD::AssimpLoadModel(*char2Animations, "Sandbox/Animations/FastRun.dae", {shader});
    #else
    Assert(false && "Assimp Not Supported");
    #endif
    //char2Model = OD::AssimpLoadModel("res/gltf/Woman.gltf", shader, &char2Clips);
    
    char2Anim.mAnimatedPose = char2Model->skeleton.GetRestPose();
    char2Anim.mPosePalette.resize(char2Model->skeleton.GetRestPose().Size());
    char2Anim.mModel.LocalPosition(Vector3(3, 0, 0));
    //char2Anim.mModel.localScale(Vector3(0.02f, 0.02f, 0.02f));
    char2Anim.mModel.LocalScale(Vector3(200.0f, 200.0f, 200.0f));
    //char2Anim.mModel.localEulerAngles(Vector3(0, 180, 0));

    char2Controller.SetSkeleton(char2Model->skeleton);
    char2Controller.Play(char2Model->animationClips[0].get());
    char2Controller.Update(0.0f);
    char2Controller.GetCurrentPose().GetMatrixPalette(char2Anim.mPosePalette);

    LogInfo("Char2 Clip Count: %zd", char2Model->animationClips.size());
    for(auto i: char2Model->animationClips){
        LogInfo("Char2 Clip Name: %s", i->GetName().c_str());
    }

};

void AnimationSample::OnUpdate(float deltaTime){
    if(Input::IsKeyDown(KeyCode::F)){
        LogInfo("To Next Animation");

        char1Anim.mClip += 1;
        if(char1Anim.mClip >= char1Clips.size()){
            char1Anim.mClip = 0;
        }

        char1Controller.FadeTo(&char1Clips[char1Anim.mClip], 0.5f);

        if(char2Controller.GetCurrentClip() == char2Model->animationClips[0].get()){
            char2Controller.FadeTo(char2Animations->animationClips[0].get(), 0.5f);
        } else {
            char2Controller.FadeTo(char2Model->animationClips[0].get(), 0.5f);
        }
    }

    float char1T = char1Anim.mPlayback;
    float char2T = char2Anim.mPlayback;

    if(animate && Application::DeltaTime() > 0){
        char1T += Application::DeltaTime();  
        char2T += Application::DeltaTime(); 
    }

    //char1Anim.mAnimatedPose = char1Skeleton.GetRestPose();
    //char1Anim.mPlayback = char1Clips[char1Anim.mClip].Sample(char1Anim.mAnimatedPose, char1T);
    //char1Anim.mAnimatedPose.GetMatrixPalette(char1Anim.mPosePalette);

    char1Controller.Update(Application::DeltaTime());
    char1Controller.GetCurrentPose().GetMatrixPalette(char1Anim.mPosePalette);

    char2Controller.Update(Application::DeltaTime());
    char2Controller.GetCurrentPose().GetMatrixPalette(char2Anim.mPosePalette, char2Model->skeleton.GetInvBindPose());

    //char2Anim.mPlayback = char2Model->animationClips[char2Anim.mClip]->Sample(char2Anim.mAnimatedPose, char2T);
    //char2Anim.mAnimatedPose.GetMatrixPalette(char2Anim.mPosePalette, char2Model->skeleton.GetInvBindPose());
    //char2Model->skeleton.GetBindPose().GetMatrixPalette(char2Anim.mPosePalette);

    camMove.OnUpdate();
};

void AnimationSample::OnRender(float deltaTime){
    cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
    cam.view = math::inverse(camTransform.GetLocalModelMatrix());

    Graphics::Begin();
    Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
    Graphics::SetCamera(cam);

    AlignedVector<Matrix4>& invBindPose = char1Skeleton.GetInvBindPose();
    for(int i = 0; i < char1Anim.mPosePalette.size(); ++i){
        char1Anim.mPosePalette[i] = char1Anim.mPosePalette[i] * invBindPose[i];
    }

    //mat->SetMatrix4("animated", char1Anim.mPosePalette);
    mat->SetTexture("mainTex", texture);
    
    for(auto i: char1Meshs){
        //Graphics::SetDefaultShaderData(*shader, char1Anim.mModel.GetLocalModelMatrix());
        /*SubShader::Bind(*shader);
        Graphics::SetProjectionViewMatrix(*shader);
        Graphics::SetModelMatrix(*shader, char1Anim.mModel.GetLocalModelMatrix());
        Graphics::DrawMeshRaw(*i);*/
        mat->EnableKeyword("SKINNED");
        Graphics::DrawMeshSkinned(*i, *mat, char1Anim.mModel.GetLocalModelMatrix(), &char1Anim.mPosePalette[0], char1Anim.mPosePalette.size());
    }

    for(int i = 0; i < char1Anim.mAnimatedPose.Size(); i++){
        if(char1Anim.mAnimatedPose.GetParent(i) < 0) continue;
        Vector3 p0 = char1Anim.mAnimatedPose.GetGlobalTransform(i).LocalPosition();
        Vector3 p1 = char1Anim.mAnimatedPose.GetGlobalTransform(char1Anim.mAnimatedPose.GetParent(i)).LocalPosition();
        Graphics::DrawLine(p0, p1, Vector3(0, 1, 0), 1);
    }

    //char2Model->materials[0]->UpdateDatas();
    //Material::SubmitGraphicDatas(*char2Model->materials[0]);
    //Graphics::BindMaterial(*char2Model->materials[0]);

    //SubShader::Bind(*char2Model->materials[0]->GetShader());
    //char2Model->materials[0]->GetShader()->SetMatrix4("animated", char2Anim.mPosePalette);

    /*for(auto i: char2Model->meshs){
        Renderer::DrawMesh(
            *i, 
            char2Anim.mModel.GetLocalModelMatrix(), 
            *char2Model->materials[0]->shader()
        );
    }*/

    for(auto i: char2Model->renderTargets){
        Matrix4 m = 
            char2Anim.mModel.GetLocalModelMatrix() * 
            char2Model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);

        //m = char2Anim.mModel.GetLocalModelMatrix();

        //LogInfo("Bind Pose Index: %d", i.bindPoseIndex);

        //Graphics::SetDefaultShaderData(*char2Model->materials[i.materialIndex]->GetShader(), m);
        /*SubShader::Bind(*char2Model->materials[i.materialIndex]->GetShader());
        Graphics::SetProjectionViewMatrix(*char2Model->materials[i.materialIndex]->GetShader());
        Graphics::SetModelMatrix(*char2Model->materials[i.materialIndex]->GetShader(), m);
        Graphics::DrawMeshRaw(*char2Model->meshs[i.meshIndex]);*/

        char2Model->materials[i.materialIndex]->EnableKeyword("SKINNED");
        Graphics::DrawMeshSkinned(
            *char2Model->meshs[i.meshIndex], 
            *char2Model->materials[i.materialIndex], 
            m,
            &char1Anim.mPosePalette[0], 
            char1Anim.mPosePalette.size()
        );
    }

    for(int i = 0; i < char2Anim.mAnimatedPose.Size(); i++){
        if(char2Anim.mAnimatedPose.GetParent(i) < 0) continue;
        Vector3 p0 = char2Anim.mAnimatedPose.GetGlobalTransform(i).LocalPosition();
        Vector3 p1 = char2Anim.mAnimatedPose.GetGlobalTransform(char2Anim.mAnimatedPose.GetParent(i)).LocalPosition();
        Graphics::DrawLine(p0, p1, Vector3(0, 0, 1), 1);
    }

    Graphics::End();
};

void AnimationSample::OnGUI(){
    /*ImGuizmo::Enable(true);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();*/

    ImGui::Begin("Animation");
    ImGui::Checkbox("Animated", &animate);
    ImGui::SliderFloat("Char1_T", &char1Anim.mPlayback, 0, char1Clips[char1Anim.mClip].GetEndTime());
    ImGui::SliderFloat("Char2_T", &char2Anim.mPlayback, 0, char2Model->animationClips[char2Anim.mClip]->GetEndTime());
    ImGui::End();

    /*
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
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
    */
};

void AnimationSample::OnResize(int width, int height){};
void AnimationSample::OnExit(){}