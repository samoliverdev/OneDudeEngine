#include "ParticleSystem.h"
#include "Standard/Ultis/Ultis.h"
#include <OD/Core/Time.h>
#include <OD/Graphics/Common.h>
#include <OD/Graphics/Geometry.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/Model.h>
#include <OD/Graphics/Mesh.h>
#include <OD/Core/ImGui.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp> // glm::linearRand, glm::sphericalRand
#include <algorithm>
#include <cmath>
#include <sstream>
#include <cereal/archives/binary.hpp>

namespace Standard{

namespace {
constexpr float MaxPreviewTime = 120.0f;
constexpr float PreviewStep = 1.0f / 120.0f;

float SampleParticleRange(std::mt19937* random, float min, float max){
    if(random == nullptr) return Ultis::RandomRange(min, max);
    return min + std::generate_canonical<float, 24>(*random) * (max - min);
}
}

void SpawnModule::OnGui(){
    if(ImGui::CollapsingHeader("Spawn")){
        ImGui::DragInt("singleBustEmiterCount", &singleBustEmiterCount);
        ImGui::DragFloat("overTimeEmiterRate", &overTimeEmiterRate);
    }
}

void SpawnModule::OnStartSpawnUpdate(ParticleEmiter& system, std::mt19937* random){
    int singleNewParticles = singleBustEmiterCount;
    for(int i = 0; i < singleNewParticles; i++){
        system.SpawnNewParticle(random);
    }
}

void SpawnModule::OnSpawnUpdate(ParticleEmiter& system, float delta, std::mt19937* random){
    emissionAccumulator += delta * overTimeEmiterRate;

    int newParticles = (int)emissionAccumulator;
    emissionAccumulator -= newParticles; // keep the leftover fraction

    for (int i = 0; i < newParticles; i++){
        system.SpawnNewParticle(random);
    }
}

void InitialLifeModule::OnGui(){
    if(ImGui::CollapsingHeader("InitialLife")){
        ImGui::DragFloat("minLife", &minLife);
        ImGui::DragFloat("maxLife", &maxLife);
    }
}

void InitialLifeModule::OnInitParticle(ParticleData& particle, std::mt19937* random){
    particle.life = SampleParticleRange(random, minLife, maxLife);
}

void InitialVelocityModule::OnGui(){
    if(ImGui::CollapsingHeader("InitialVelocity")){
        ImGui::DragFloat3("minVelocity", &minVelocity.x);
        ImGui::DragFloat3("maxVelocity", &maxVelocity.x);
    }
}

void InitialVelocityModule::OnInitParticle(ParticleData& particle, std::mt19937* random){
    particle.vel = Vector3(
        SampleParticleRange(random, minVelocity.x, maxVelocity.x),
        SampleParticleRange(random, minVelocity.y, maxVelocity.y),
        SampleParticleRange(random, minVelocity.z, maxVelocity.z)
    ); 
}

void InitialRotationModule::OnGui(){
    if(ImGui::CollapsingHeader("InitialRotation")){
        ImGui::DragFloat("minSize", &minRotation);
        ImGui::DragFloat("maxSize", &maxRotation);
    }
}

void InitialRotationModule::OnInitParticle(ParticleData& particle, std::mt19937* random){
    particle.rotation = SampleParticleRange(random, minRotation, maxRotation);
}

void InitialSizeModule::OnGui(){
    if(ImGui::CollapsingHeader("InitialSize")){
        ImGui::Checkbox("uniforSize", &uniforSize);

        if(uniforSize){
            ImGui::DragFloat("minSize", &minSize.x);
            ImGui::DragFloat("maxSize", &maxSize.x);
        } else {
            ImGui::DragFloat3("minSize", &minSize.x);
            ImGui::DragFloat3("maxSize", &maxSize.x);
        }
    }
}

void InitialSizeModule::OnInitParticle(ParticleData& particle, std::mt19937* random){
    if(uniforSize){
        particle.size = Vector3(SampleParticleRange(random, minSize.x, maxSize.x));
    } else {
        particle.size = Vector3(
            SampleParticleRange(random, minSize.x, maxSize.x),
            SampleParticleRange(random, minSize.y, maxSize.y),
            SampleParticleRange(random, minSize.z, maxSize.z)
        ); 
    }
}

void InitialColorModule::OnGui(){
    if(ImGui::CollapsingHeader("InitialColor")){
        ImGui::ColorEdit4("color", &color.r, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
    }
}

void InitialColorModule::OnInitParticle(ParticleData& particle, std::mt19937*){
    particle.color = color;
}

void UpdaterModule::OnGui(){
    if(ImGui::CollapsingHeader("Updater")){
        ImGui::DragFloat("gravityModifier", &gravityModifier);
    }
}

void UpdaterModule::OnParticleUpdate(ParticleData& particle, ParticleRunningData& runningData){
    particle.lastPos = particle.pos;
    particle.vel += Vector3(0.0f,-9.81f, 0.0f) * (runningData.delta * gravityModifier);
    particle.pos += particle.vel * runningData.delta;
}

void SizeOverLifetimeModule::OnGui(){
    if(ImGui::CollapsingHeader("SizeOverLifeTime")){
        ImGui::Checkbox("enable", &enable);
        ImGui::DragFloat("minSizeScale", &minSizeScale);
        ImGui::DragFloat("maxSizeScale", &maxSizeScale);

        DrawCurvePreview(curve, ImVec2(100, 25), &showCurve);

        if(showCurve){
            ImGui::OpenPopup("Curve Editor");
        }

        if(ImGui::BeginPopupModal("Curve Editor", &showCurve, ImGuiWindowFlags_AlwaysAutoResize)){
            DrawAnimationCurveEditor(curve, ImVec2(400, 200));
            ImGui::EndPopup();
        }
    }
}

void SizeOverLifetimeModule::OnParticleUpdate(ParticleData& p, ParticleRunningData& runningData){
    if(enable == false) return;
    //p.size = p.startSize * math::mix(minSizeScale, maxSizeScale, runningData.lifetime);
    p.size = p.startSize * curve.Evaluate(runningData.lifetime);
}

void ColorOverLifetimeModule::OnGui(){
    if(ImGui::CollapsingHeader("ColorOverLifetimeModule")){
        ImGui::Checkbox("enable", &enable);
        ImGui::ColorEdit4("colorA", &colorA, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
        ImGui::ColorEdit4("colorB", &colorB, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

        int32_t stateID = 10;
	    static ImGradientHDRTemporaryState tempState;

	    bool isMarkerShown = true;
	    ImGradientHDR(stateID, gradient, tempState, isMarkerShown);

        if (ImGui::IsItemHovered()){
            ImGui::SetTooltip("Gradient");
        }

        if (tempState.selectedMarkerType == ImGradientHDRMarkerType::Color){
            auto selectedColorMarker = gradient.GetColorMarker(tempState.selectedIndex);
            if(selectedColorMarker != nullptr){
                ImGui::ColorEdit3("Color", selectedColorMarker->Color.data(), ImGuiColorEditFlags_Float);
                ImGui::DragFloat("Intensity", &selectedColorMarker->Intensity, 0.1f, 0.0f, 100.0f, "%f", 1.0f);
            }
        }

        if(tempState.selectedMarkerType == ImGradientHDRMarkerType::Alpha){
            auto selectedAlphaMarker = gradient.GetAlphaMarker(tempState.selectedIndex);
            if(selectedAlphaMarker != nullptr){
                ImGui::DragFloat("Alpha", &selectedAlphaMarker->Alpha, 0.1f, 0.0f, 1.0f, "%f", 1.0f);
            }
        }

        if(tempState.selectedMarkerType != ImGradientHDRMarkerType::Unknown){
            if(ImGui::Button("Delete")){
                if(tempState.selectedMarkerType == ImGradientHDRMarkerType::Color){
                    gradient.RemoveColorMarker(tempState.selectedIndex);
                    tempState = ImGradientHDRTemporaryState{};
                } else if(tempState.selectedMarkerType == ImGradientHDRMarkerType::Alpha){
                    gradient.RemoveAlphaMarker(tempState.selectedIndex);
                    tempState = ImGradientHDRTemporaryState{};
                }
            }
        }
    }
}

void ColorOverLifetimeModule::OnParticleUpdate(ParticleData& p, ParticleRunningData& runningData){
    if(enable == false) return;
    //p.color = Color::Lerp(colorA, colorB, runningData.lifetime);

    auto color = gradient.GetCombinedColor(runningData.lifetime);
    p.color = Color(color[0], color[1], color[2], color[3]);//TODO: Revise this "color[3] * 255.0f" quick fix
}

void LimitVelocityOverLifetimeModule::OnGui(){
    if(ImGui::CollapsingHeader("LimitVelocityOverLifetimeModule")){
        ImGui::Checkbox("enable", &enable);
        ImGui::Checkbox("separateAxes", &separateAxes);
        ImGui::DragFloat("speed", &speed);
        ImGui::DragFloat3("speedXYZ", &speedXYZ.x);
        ImGui::DragFloat("dampen", &dampen);
    }
}

void LimitVelocityOverLifetimeModule::OnParticleUpdate(
    ParticleData& particle,
    ParticleRunningData& runningData
){
    if(!enable) return;

    if(separateAxes){
        Vector3 targetVel = particle.vel;

        targetVel.x = glm::clamp(targetVel.x, -speedXYZ.x, speedXYZ.x);
        targetVel.y = glm::clamp(targetVel.y, -speedXYZ.y, speedXYZ.y);
        targetVel.z = glm::clamp(targetVel.z, -speedXYZ.z, speedXYZ.z);

        if(dampen <= 0.0f){
            particle.vel = targetVel;
        } else {
            float t = glm::clamp(dampen * runningData.delta * 10.0f, 0.0f, 1.0f);
            particle.vel = glm::mix(particle.vel, targetVel, t);
        }

        return;
    }

    float len = glm::length(particle.vel);

    if(len > speed && len > 0.00001f){
        Vector3 targetVel = particle.vel / len * speed;

        if(dampen <= 0.0f){
            particle.vel = targetVel;
        } else {
            float t = glm::clamp(dampen * runningData.delta * 10.0f, 0.0f, 1.0f);
            particle.vel = glm::mix(particle.vel, targetVel, t);
        }
    }
}

void CollisionPhysicModule::OnGui(){
    if(ImGui::CollapsingHeader("CollisionPhysicModule")){
        ImGui::Checkbox("enable", &enable);
        ImGui::DragFloat("rayOffset", &rayOffset);
        ImGui::DragInt("maxCollisionsCount", &maxCollisionsCount);
        ImGui::DrawLayerMask("mask", mask);
    }
}

void CollisionPhysicModule::OnParticleUpdate(ParticleData& particle, ParticleRunningData& runningData){
    if(enable == false) return;
    if(scene->Running() == false) return;

    if(maxCollisionsCount > 0){
        if(particle.handleCollision == false && *curParticleCollisionCount < maxCollisionsCount){
            particle.handleCollision = true;
            *curParticleCollisionCount += 1;
        }
    } else {
        if(particle.handleCollision == false){
            particle.handleCollision = true;
            *curParticleCollisionCount += 1;
        }
    }
    if(particle.handleCollision == false) return;

    Vector3 pos = particle.lastPos;
    Vector3 dir = (particle.pos - particle.lastPos);
    dir += math::normalize(dir) * rayOffset;

    if(isGlobalSpace == false){
        pos = globalTrans.TransformPoint(pos); //Vector3(worldModel * Vector4(pos, 1));
        dir = globalTrans.TransformDirection(dir); //Vector3(worldModel * Vector4(dir, 0));
    }

    RayResult result;
    if(physicsSystem->Raycast(pos, dir, result, mask)){
        particle.life = 0;
        //LogInfo("OnCollision");
        onCollision.Invoke(source, result);
    }
}

void RendererModule::OnGui(){
    if(ImGui::CollapsingHeader("RendererModule")){
        ImGui::DrawAsset<Material>("material", material);
        ImGui::DrawAsset<Model>("model", model);

        ImGui::DrawEnumCombo<RendererModule::Orientation>("Orientation", &orientation);
    }
}

//////////////////////////////////////////////////////////

void ParticleEmiter::OnGui(){
    ImGui::Checkbox("isLooping", &isLooping);
    ImGui::DragFloat("duration", &duration);

    if(ImGui::DragInt("maxParticles", &maxParticles)){
        SetMaxParticle(maxParticles);
        //Reset();
    }

    if(ImGui::DragFloat("delay", &delay)){
        //Reset();
    }

    if(ImGui::DrawEnumCombo<SimulationSpace>("simulationSpace", &simulationSpace)){}

    spawnModule.OnGui();

    initialLifeModule.OnGui();
    initialVelocityModule.OnGui();
    initialRotationModule.OnGui();
    initialSizeModule.OnGui();
    initialColorModule.OnGui();

    updaterModule.OnGui();
    sizeOverLifetimeModule.OnGui();
    colorOverLifetimeModule.OnGui();
    limitVelocityOverLifetimeModule.OnGui();
    collisionPhysicModule.OnGui();
    
    rendererModule.OnGui();

    /*ImGui::Separator();

    ImVec4 color = ImVec4(1.0f, 0.7f, 0.2f, 1.0f); // orange
    ImGui::TextColored(color, "State %s", std::string(magic_enum::enum_name(state)).c_str());
    ImGui::Text("RunTime: %f", runningTime);
    ImGui::Text("ParticleCounts: %d", particlesCount);

    if(state == State::Running && ImGui::Button("Stop")){
        Stop();
    } else if(state == State::Stop && ImGui::Button("Play")){
        Play();
    }*/
}

void ParticleEmiter::BindModules(){
    spawnModules.clear();
    initParticleModules.clear();
    updateModules.clear();

    spawnModules.push_back(&spawnModule);
    
    initParticleModules.push_back(&initialLifeModule);
    initParticleModules.push_back(&initialVelocityModule);
    initParticleModules.push_back(&initialRotationModule);
    initParticleModules.push_back(&initialSizeModule);
    initParticleModules.push_back(&initialColorModule);

    updateModules.push_back(&limitVelocityOverLifetimeModule);
    updateModules.push_back(&updaterModule);
    updateModules.push_back(&sizeOverLifetimeModule);
    updateModules.push_back(&colorOverLifetimeModule);
    //updateModules.push_back(&limitVelocityOverLifetimeModule);
    updateModules.push_back(&collisionPhysicModule);
}

void ParticleEmiter::Reset(){
    runningTime = 0;
    curDelayTime = delay;
    hasStarted = false;
    spawnModule.Reset();

    currentParticleIndex = 0;
    particlesCount = 0;
    curParticleCollisionCount = 0;
    freeParticles.clear();
    for(int i = 0; i < particles.size(); i++){
        particles[i].life = 0;
    }
}

void ParticleEmiter::Play(){
    state = State::Running;
    Reset();
}

void ParticleEmiter::Stop(){
    state = State::Stop;
    Reset();
}

ParticleEmiter::State ParticleEmiter::CurState(){
    return state;
}

void ParticleEmiter::SetMaxParticle(int inmaxParticles){
    maxParticles = inmaxParticles;
    particles.resize(maxParticles);
}

int ParticleEmiter::GetNewParticle(){
    int newParticle = -1;

	if(currentParticleIndex < particles.size()){
		newParticle = currentParticleIndex;
		currentParticleIndex++;
	} else if (freeParticles.size() > 0) {
		newParticle = freeParticles[freeParticles.size()-1];
        freeParticles.pop_back();
	} else {
        return newParticle;
    }

    particles[newParticle] = ParticleData();
	return newParticle;
}

void ParticleEmiter::FreeParticle(int index){
    freeParticles.push_back(index);
}

/*void ParticleSystem::SpawnParticle(Particle &particle){
    particle.pos = Vector3Zero; //Vector3(Ultis::RandomRange(-2.0f, 2.0f), Ultis::RandomRange(0.0f, 2.0f), Ultis::RandomRange(-2.0f, 2.0f));
    particle.life = Ultis::RandomRange(startLifeTimeMinMax.x, startLifeTimeMinMax.y); //1.0f;
    particle.startLife = particle.life;
    particle.vel = Vector3(
        Ultis::RandomRange(shape.minMaxVelX.x, shape.minMaxVelX.y), 
        Ultis::RandomRange(shape.minMaxVelY.x, shape.minMaxVelY.y), 
        Ultis::RandomRange(shape.minMaxVelZ.x, shape.minMaxVelZ.y)
    ); 
    particle.vel = math::normalizeSafe(particle.vel) * Ultis::RandomRange(startSpeedTimeMinMax.x, startSpeedTimeMinMax.y); 
    
    float targetSize = Ultis::RandomRange(startSizeTimeMinMax.x, startSizeTimeMinMax.y);
    particle.size = Vector3(targetSize);
    particle.startSize = particle.size;
}*/

void ParticleEmiter::SpawnNewParticle(std::mt19937* random){
    int newIndex = GetNewParticle();
    if(newIndex == -1) return;

    ParticleData& p = particles[newIndex];
    for(auto* i: initParticleModules) i->OnInitParticle(p, random);
    p.startSize = p.size;
    p.startLife = p.life;
    if(simulationSpace == SimulationSpace::WorldSpace){
        //p.pos += currentGlobalTrans.Position();
        p.pos = currentGlobalTrans.TransformPoint(p.pos); 
        p.vel = currentGlobalTrans.TransformDirection(p.vel); 
    }
}

void ParticleEmiter::Update(Scene& scene, Entity e, TransformComponent& trans, Vector3 camPos){
    if(state != State::Running) return;

    PrepareUpdate(scene, e, trans);
    Simulate(Time::DeltaTime(), camPos);
}

void ParticleEmiter::PrepareUpdate(Scene& scene, Entity e, TransformComponent& trans){
    currentGlobalTrans = trans.ToTransform();
    collisionPhysicModule.curParticleCollisionCount = &curParticleCollisionCount;
    collisionPhysicModule.isGlobalSpace = simulationSpace == SimulationSpace::WorldSpace;
    collisionPhysicModule.globalTrans = currentGlobalTrans; 
    collisionPhysicModule.worldModel = currentGlobalTrans.GetModelMatrix(); 
    collisionPhysicModule.scene = &scene;
    collisionPhysicModule.source = e;
    collisionPhysicModule.physicsSystem = scene.GetSystem<PhysicsSystem>();

    if(spawnModules.size() == 0) BindModules();
    if(particles.size() != maxParticles) SetMaxParticle(maxParticles);
}

void ParticleEmiter::Simulate(float delta, Vector3 camPos, std::mt19937* previewRandom){
    if(state != State::Running) return;

    if(curDelayTime > 0.0f){
        const float elapsedDelay = std::min(delta, curDelayTime);
        curDelayTime -= elapsedDelay;
        delta -= elapsedDelay;
        if(curDelayTime > 0.0f) return;
    }

    const float emissionDelta = isLooping ? delta : std::clamp(duration - runningTime, 0.0f, delta);
    if(!hasStarted){
        hasStarted = true;
        for(auto* module: spawnModules) module->OnStartSpawnUpdate(*this, previewRandom);
    }
    for(auto* module: spawnModules) module->OnSpawnUpdate(*this, emissionDelta, previewRandom);
    runningTime += delta;

    particlesCount = 0;
    for(size_t index = 0; index < particles.size(); index++){
        auto& particle = particles[index];
        if(particle.IsDead()) continue;

        const float lifetime = std::clamp(1.0f - particle.life / particle.startLife, 0.0f, 1.0f);
        ParticleRunningData runningData{delta, lifetime};
        for(auto* module: updateModules){
            if(previewRandom != nullptr && module == &collisionPhysicModule) continue;
            module->OnParticleUpdate(particle, runningData);
        }
        particle.life -= delta;
        if(!particle.IsDead()){
            particle.cameradistance = math::length2(particle.pos - camPos);
            particlesCount++;
        }else{
            if(particle.handleCollision) curParticleCollisionCount--;
            particle.cameradistance = -1.0f;
            FreeParticle(static_cast<int>(index));
        }
    }

    const bool emissionFinished = runningTime >= duration || spawnModule.overTimeEmiterRate <= 0.0f;
    if(!isLooping && emissionFinished && particlesCount == 0){
        Stop();
    }
}

void ParticleEmiter::Sort(){
    //std::sort(particles.begin(), particles.end(), [](ParticleData& a, ParticleData& b){
    //    return a.cameradistance > b.cameradistance;
    //});
    //std::sort(&particles[0], &particles[particles.size()]);
}

glm::mat4 MakeBillboard(const glm::vec3& objectPos, const glm::mat4& view, const glm::mat4& proj){
    // Extract camera right, up and forward vectors from the view matrix
    glm::vec3 camRight   = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 camUp      = glm::vec3(view[0][1], view[1][1], view[2][1]);
    glm::vec3 camForward = -glm::vec3(view[0][2], view[1][2], view[2][2]); // negate because view looks toward -Z

    glm::mat4 model(1.0f);
    model[0] = glm::vec4(camRight,   0.0f);
    model[1] = glm::vec4(camUp,      0.0f);
    model[2] = glm::vec4(-camForward, 0.0f);
    model[3] = glm::vec4(objectPos,  1.0f);

    return model;
}

glm::mat4 MakeBillboard(const glm::vec3& objectPos, const glm::mat4& view, float rotationRad){
    glm::vec3 camRight   = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));
    glm::vec3 camUp      = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
    glm::vec3 camForward = -glm::normalize(glm::vec3(view[0][2], view[1][2], view[2][2]));

    glm::vec3 normal = -camForward; // face camera

    float c = cos(rotationRad);
    float s = sin(rotationRad);

    glm::vec3 right = camRight * c + camUp * s;
    glm::vec3 up    = camUp * c - camRight * s;

    glm::mat4 model(1.0f);
    model[0] = glm::vec4(right,  0.0f);
    model[1] = glm::vec4(up,     0.0f);
    model[2] = glm::vec4(normal, 0.0f);
    model[3] = glm::vec4(objectPos, 1.0f);

    return model;
}

glm::mat4 MakeBillboardViewPlusVelocity(
    const glm::vec3& objectPos,
    const glm::vec3& velocity,
    const glm::mat4& view)
{
    // Extract camera basis vectors from view matrix
    glm::vec3 camRight   = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 camUp      = glm::vec3(view[0][1], view[1][1], view[2][1]);
    glm::vec3 camForward = -glm::vec3(view[0][2], view[1][2], view[2][2]);

    // Convert velocity to camera space (important!)
    glm::vec3 velCam = glm::vec3(view * glm::vec4(velocity, 0.0f));
    if (glm::length2(velCam) < 1e-6f)
        velCam = glm::vec3(0.0f, 1.0f, 0.0f); // fallback

    // Local Y axis → particle velocity direction in camera space
    glm::vec3 axisY = glm::normalize(velCam);

    // Local Z axis → facing camera
    glm::vec3 axisZ = glm::vec3(0, 0, -1); // billboard always faces camera

    // Local X axis → cross(Y, Z)
    //glm::vec3 axisX = glm::normalize(glm::cross(axisY, axisZ));
    //axisY = glm::normalize(glm::cross(axisZ, axisX));

    glm::vec3 axisX = glm::normalize(glm::cross(axisZ, axisY));
    axisY = glm::normalize(glm::cross(axisX, axisZ));

    // Build rotation in camera space
    glm::mat3 rot;
    rot[0] = axisX;
    rot[1] = axisY;
    rot[2] = axisZ;

    // Convert from camera space back to world space
    glm::mat3 camRot = glm::mat3(camRight, camUp, camForward);
    glm::mat3 worldRot = camRot * rot;

    // Build final matrix
    glm::mat4 model(1.0f);
    model[0] = glm::vec4(worldRot[0], 0.0f);
    model[1] = glm::vec4(worldRot[1], 0.0f);
    model[2] = glm::vec4(worldRot[2], 0.0f);
    model[3] = glm::vec4(objectPos, 1.0f);

    return model;
}

void SortDrawDataByParticleDistance(std::vector<Matrix4>& drawData,const std::vector<ParticleData>& particles){
    size_t N = drawData.size();
    if(N != particles.size()) return; // sanity check

    // create an index array
    std::vector<size_t> indices(N);
    for(size_t i = 0; i < N; i++) indices[i] = i;

    // sort indices based on particle cameradistance (descending)
    std::sort(indices.begin(), indices.end(), [&particles](size_t a, size_t b){
        return particles[a].cameradistance > particles[b].cameradistance;
    });

    // rearrange drawData according to sorted indices
    std::vector<Matrix4> tmp = drawData;
    for(size_t i = 0; i < N; i++) {
        drawData[i] = tmp[indices[i]];
    }
}

void ParticleEmiter::SubmitDrawData(InstancingBuffer& buffer, const Matrix4& root, const Camera* cam){
    drawData.resize(particlesCount);
    int i = 0;
    for(int _i = 0; _i < particles.size(); _i++){
        if(particles[_i].life <= 0) continue;

        Transform t(particles[_i].pos, QuaternionIdentity, particles[_i].size);

        auto targetModelMatrix = t.GetModelMatrix();

        Assert(cam != nullptr);
        if(rendererModule.orientation == RendererModule::Orientation::View){
            //targetModelMatrix = MakeBillboard(t.Position(), cam->view, cam->projection) * glm::scale(glm::mat4(1.0f), t.Scale());
            targetModelMatrix = MakeBillboard(t.Position(), cam->view, math::radians(particles[_i].rotation)) * glm::scale(glm::mat4(1.0f), t.Scale());
        }

        if(rendererModule.orientation == RendererModule::Orientation::Velocity){
            glm::vec3 dir = math::normalize(particles[_i].vel);  
            glm::quat q = glm::rotation(glm::vec3(0, 1, 0), dir);
            targetModelMatrix = Mathf::TRS(t.Position(), q, t.Scale());
        }

        if(rendererModule.orientation == RendererModule::Orientation::ViewPlusVelocity){
            glm::mat4 model = MakeBillboardViewPlusVelocity(t.Position(), particles[_i].vel, cam->view);
            
            // Apply stretch factor (based on speed)
            float speed = glm::length(particles[_i].vel);
            glm::vec3 scale = t.Scale();
            //scale.y *= (1.0f + speed * stretchAmount); // e.g., stretchAmount = 0.05f

            targetModelMatrix = model * glm::scale(glm::mat4(1.0f), scale);
        }
        
        if(simulationSpace == SimulationSpace::Local){
            auto m = root * targetModelMatrix;
            drawData[i] = Matrix4(math::row(m, 0), math::row(m, 1), math::row(m, 2), ToLinear((Vector4)particles[_i].color));
        } else {
            auto m = targetModelMatrix;
            drawData[i] = Matrix4(math::row(m, 0), math::row(m, 1), math::row(m, 2), ToLinear((Vector4)particles[_i].color));
        }

        i += 1;
    }

    SortDrawDataByParticleDistance(drawData, particles);

    buffer.SetData(drawData.data(), particlesCount);

    glm::mat4 m(1.0f);
    m[3] = glm::vec4(10.0f, 20.0f, 30.0f, 1.0f);

    auto r0 = math::row(m, 0);
    auto r1 = math::row(m, 1);
    auto r2 = math::row(m, 2);
    auto r3 = math::row(m, 3);

    Assert(r0 == glm::vec4(1, 0, 0, 10));
    Assert(r1 == glm::vec4(0, 1, 0, 20));
    Assert(r2 == glm::vec4(0, 0, 1, 30));
    Assert(r3 == glm::vec4(0, 0, 0, 1));
}

void ParticleSystem::OnGui(){
    const bool playing = IsPlaying();
    if(paused){
        if(ImGui::Button("Resume") && !requestedTime.has_value()){
            if(std::any_of(emiters.begin(), emiters.end(), [](const ParticleEmiter& emitter){
                return emitter.state == ParticleEmiter::State::Running;
            })){
                paused = false;
            }else{
                Play();
            }
        }
    }else if(playing){
        if(ImGui::Button("Pause")) paused = true;
    }else{
        if(ImGui::Button("Play")) Play();
    }
    ImGui::SameLine();
    if(ImGui::Button("Stop")) Stop();

    float time = requestedTime.value_or(currentTime);
    if(ImGui::DragFloat("Time (s)", &time, 0.01f, 0.0f, MaxPreviewTime, "%.3f", ImGuiSliderFlags_AlwaysClamp)
        && std::isfinite(time)){
        requestedTime = time;
        paused = true;
    }
    if(ImGui::IsItemHovered()){
        ImGui::SetTooltip("Drag to scrub, or Ctrl-click to enter seconds.");
    }
    ImGui::Checkbox("Resimulate", &resimulate);
    if(ImGui::IsItemHovered()){
        ImGui::SetTooltip("Apply emitter edits at the current preview time (up to 120 seconds).");
    }
    ImGui::TextUnformatted(paused ? "Paused" : IsPlaying() ? "Playing" : "Stopped");
    ImGui::Separator();

    // Compare serialized settings so custom controls such as the HDR gradient
    // participate without relying on individual ImGui widgets to report edits.
    const auto captureSettings = [this](){
        std::ostringstream stream(std::ios::binary);
        cereal::BinaryOutputArchive archive(stream);
        archive(emiters);
        return stream.str();
    };
    const bool trackEdits = resimulate && currentTime <= MaxPreviewTime && (paused || IsPlaying());
    const auto beforeEdits = trackEdits ? captureSettings() : std::string{};

    for(int i = 0; i < emiters.size(); i++){
        std::string name = "Emitter " + std::to_string(i);

        // Hardcoded color lookup table (10 colors)
        static const ImVec4 headerColors[10] = {
            {0.9f, 0.4f, 0.3f, 0.4f}, // red-ish
            {0.9f, 0.7f, 0.3f, 0.4f}, // orange
            {0.9f, 0.9f, 0.3f, 0.4f}, // yellow
            {0.5f, 0.9f, 0.3f, 0.4f}, // green
            {0.3f, 0.9f, 0.6f, 0.4f}, // teal
            {0.3f, 0.7f, 0.9f, 0.4f}, // blue
            {0.5f, 0.4f, 0.9f, 0.4f}, // purple
            {0.8f, 0.3f, 0.8f, 0.4f}, // magenta
            {0.6f, 0.6f, 0.6f, 0.4f}, // gray
            {1.0f, 1.0f, 1.0f, 0.3f}, // white (default)
        };

        // Clamp the index to max 9
        int colorIndex = (i < 10) ? i : 9;
        const ImVec4& baseColor = headerColors[colorIndex];

        // Slight variations for hover/active
        ImVec4 hoverColor = ImVec4(baseColor.x, baseColor.y, baseColor.z, baseColor.w + 0.1f);
        ImVec4 activeColor = ImVec4(baseColor.x, baseColor.y, baseColor.z, baseColor.w + 0.2f);

        // Push colors
        ImGui::PushStyleColor(ImGuiCol_Header, baseColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hoverColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, activeColor);

        bool show = ImGui::CollapsingHeader(name.c_str());

         // Popup menu for right-click
        if(ImGui::BeginPopupContextItem(name.c_str())){
            if(ImGui::MenuItem("Move Up", nullptr, false, i > 0)) std::swap(emiters[i], emiters[i - 1]);
            if(ImGui::MenuItem("Move Down", nullptr, false, i < emiters.size() - 1)) std::swap(emiters[i], emiters[i + 1]);
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(3);

        if(show) emiters[i].OnGui();
    }

    ImGui::Separator();

    if(trackEdits && !requestedTime && captureSettings() != beforeEdits){
        requestedTime = currentTime;
    }

    if(ImGui::Button("PushEmiter")){
        Stop();
        emiters.push_back(ParticleEmiter());
    }

    if(ImGui::Button("PopEmiter")){
        Stop();
        if(emiters.size() > 1) emiters.pop_back();
    }

    ImGui::Separator();

    int particlesCount = 0;
    int particlesCollisionCount = 0;
    for(const auto& emitter: emiters){
        particlesCount += emitter.particlesCount;
        particlesCollisionCount += emitter.curParticleCollisionCount;
    }
    ImGui::Text("ParticleCounts: %d", particlesCount);
    ImGui::Text("ParticleCollisionCounts: %d", particlesCollisionCount);
}

bool ParticleSystem::IsPlaying(){
    return !paused && std::any_of(emiters.begin(), emiters.end(), [](const ParticleEmiter& emitter){
        return emitter.state == ParticleEmiter::State::Running;
    });
}

void ParticleSystem::Play(){
    paused = false;
    requestedTime.reset();
    currentTime = 0.0f;
    for(auto& i: emiters) i.Play();
}

void ParticleSystem::Stop(){
    paused = false;
    requestedTime.reset();
    currentTime = 0.0f;
    for(auto& i: emiters) i.Stop();
}

void ParticleSystem::Update(Scene& scene, Entity e, TransformComponent& trans, Vector3 camPos){
    if(requestedTime.has_value()){
        currentTime = *requestedTime;
        requestedTime.reset();
        for(size_t index = 0; index < emiters.size(); index++){
            auto& emitter = emiters[index];
            emitter.Play();
            emitter.PrepareUpdate(scene, e, trans);

            // Rebuild with the same seed whenever the inspector visits a time.
            std::mt19937 random(static_cast<std::mt19937::result_type>(index));
            emitter.Simulate(0.0f, camPos, &random);
            const int wholeSteps = static_cast<int>(currentTime / PreviewStep);
            for(int step = 0; step < wholeSteps && emitter.state == ParticleEmiter::State::Running; step++){
                emitter.Simulate(PreviewStep, camPos, &random);
            }
            const float remainder = std::max(0.0f, currentTime - wholeSteps * PreviewStep);
            if(remainder > 0.0f) emitter.Simulate(remainder, camPos, &random);
            if(currentTime > 0.0f) emitter.Simulate(0.0f, camPos, &random);
        }
        return;
    }

    if(!IsPlaying()) return;
    currentTime += Time::DeltaTime();
    for(auto& i: emiters) i.Update(scene, e, trans, camPos);
}

ParticleRendererFeature::ParticleRendererFeature(){
    material = ResourceManager::Get().Create<Material>(ResourceManager::Get().LoadByPath<Shader>("Standard/Shaders/UnlitParticleBlend.glsl"));
    material->SetEnableInstancing(true);
    material->SetFloat("smoothness", 0);
    mesh = Resource::CreateFromFile<Model>("Engine/Models/Cube.obj", ModelLoadSettings{nullptr, 1, false}); //Model::CreateFromFile("Engine/Models/Cube.obj", {nullptr, 1, false});
}

void ParticleRendererFeature::OnCollectRenderData(Scene& scene, RenderContext& context, std::vector<RenderData>& outRenderData){
    //Scene* scene = context.GetScene();
    auto cam = context.GetCamera();

    auto view = scene.GetRegistry().view<TransformComponent, ParticleComponent>();
    for(auto [entity, trans, particle]: view.each()){
        auto modelMatrix = trans.GlobalModelMatrix();
        auto pos = trans.PositionReadSafe();// .Position();

        for(int i = 0; i < particle.particleSystem.emiters.size(); i++){
            auto& emiter = particle.particleSystem.emiters[i];
            //if(emiter.state != ParticleEmiter::State::Running) continue;

            emiter.SubmitDrawData(*emiter.dataBuffer, modelMatrix, &cam);
            if(emiter.dataBuffer->Count() == 0) continue;

            Assert(emiter.rendererModule.material != nullptr);

            RenderData renderData;
            renderData.distance = math::distance(pos, cam.viewPos);
            renderData.distance -= i * 0.01f; 
            renderData.targetMatrix = modelMatrix; //trans.GlobalModelMatrix();
            renderData.aabb = transform_aabb_optimized_abs_center_extents(AABB({0, 0, 0}, 10, 10, 10), renderData.targetMatrix);
            renderData.targetMaterial = emiter.rendererModule.material->GetId(); //material.get();
            renderData.targetMesh = emiter.rendererModule.model->meshs[0]->GetId(); //mesh->meshs[0].get();
            renderData.instancingBuffer = emiter.dataBuffer == nullptr ? INVALID_RESOURCE_ID : emiter.dataBuffer->GetId();
            renderData.SetFlag(RenderData::Flag::RenderShadow, material->IsBlend() == false); //renderData.renderShadow = material->IsBlend() == false;
            renderData.customShadowPass = material->DepthPass() != -1 ? material->GetId() : INVALID_RESOURCE_ID;
            outRenderData.push_back(renderData);
        }
    }
}

template<typename Iter, typename Func>
void tf_for_each4(tf::Taskflow& taskflow, Iter begin, Iter end, size_t num_tasks = 0, Func func = {}){
    using traits = std::iterator_traits<Iter>;
    using diff_t = typename traits::difference_type;

    diff_t total_diff = std::distance(begin, end);
    if(total_diff <= 0) return;

    const size_t total = static_cast<size_t>(total_diff);

    // determine num_tasks
    if(num_tasks == 0) {
        num_tasks = std::thread::hardware_concurrency();
        if(num_tasks == 0) num_tasks = 1;
    }

    // clamp tasks to not exceed total items
    if(num_tasks > total) num_tasks = total;

    const size_t chunk_size = (total + num_tasks - 1) / num_tasks; // ceil

    // helper: advance iterator by an offset (works for random-access and forward iterators)
    auto advance_iter = [](Iter it, size_t offset) -> Iter {
        if constexpr (
            std::is_same_v<typename traits::iterator_category, std::random_access_iterator_tag> ||
            std::is_base_of_v<std::random_access_iterator_tag, typename traits::iterator_category>
        ) {
            return it + static_cast<diff_t>(offset);
        } else {
            return std::next(it, static_cast<diff_t>(offset));
        }
    };

    // create tasks: each task gets its start/end iterators and its index
    for(size_t t = 0; t < num_tasks; ++t){
        const size_t start_off = t * chunk_size;
        if(start_off >= total) break;

        const size_t end_off = std::min(total, start_off + chunk_size);

        Iter chunk_begin = advance_iter(begin, start_off);
        Iter chunk_end   = advance_iter(begin, end_off);

        // capture chunk iterators and task index by value; capture func by value to avoid dangling
        taskflow.emplace([chunk_begin, chunk_end, t, func]() mutable {
            for (Iter it = chunk_begin; it != chunk_end; ++it) {
                func(*it, t); // <-- user lambda receives (element, taskIndex)
            }
        });
    }
}

void ParticleRendererFeature::OnCollectRenderData(Scene& scene, RenderContext& context, ChunkedVector<RenderData>& data){
    //Scene* scene = context.GetScene();
    auto cam = context.GetCamera();

    auto view = scene.GetRegistry().view<TransformComponent, ParticleComponent>();
    tf_for_each4(scene.GetTaskflow(), view.begin(), view.end(), data.chunk_count(), [&](auto e, int taskIndex){
        auto [trans, particle] = view.get<TransformComponent, ParticleComponent>(e);

        for(int i = 0; i < particle.particleSystem.emiters.size(); i++){
            auto& emiter = particle.particleSystem.emiters[i];
            //if(emiter.state != ParticleEmiter::State::Running) continue;

            emiter.SubmitDrawData(*emiter.dataBuffer, trans.GlobalModelMatrix(), &cam);
            if(emiter.dataBuffer->Count() == 0) continue;

            Assert(emiter.rendererModule.material != nullptr);

            RenderData& renderData = data.GetNew(taskIndex);
            renderData.distance = math::distance(trans.Position(), cam.viewPos);
            renderData.distance -= i * 0.01f; 
            renderData.targetMatrix = trans.GlobalModelMatrix();
            renderData.aabb = transform_aabb_optimized_abs_center_extents(AABB({0, 0, 0}, 10, 10, 10), renderData.targetMatrix);
            renderData.targetMaterial = emiter.rendererModule.material->GetId(); //material.get();
            renderData.targetMesh = emiter.rendererModule.model->meshs[0]->GetId(); //mesh->meshs[0].get();
            renderData.instancingBuffer = emiter.dataBuffer == nullptr ? INVALID_RESOURCE_ID : emiter.dataBuffer->GetId();
            renderData.SetFlag(RenderData::Flag::RenderShadow, material->IsBlend() == false); //renderData.renderShadow = material->IsBlend() == false;
            renderData.customShadowPass = material->DepthPass() != -1 ? material->GetId() : INVALID_RESOURCE_ID;
        }
    });

    scene.RunAllTaskAndSync();
}

void ParticleComponent::OnGui(Entity e, Scene& scene){
    ParticleComponent& p = scene.GetComponent<ParticleComponent>(e);
    p.particleSystem.OnGui();
}

int ParticleManageSystem::Type(){ 
    return SystemType::Stand; 
} 

bool ParticleManageSystem::ExecuteAlways(){ 
    return true; 
}

void ParticleManageSystem::Update(Scene& scene){
    Vector3 camPos = Vector3Zero; 
    if(scene.GetMainCamera() != EntityNull){
        camPos = scene.GetComponent<TransformComponent>(scene.GetMainCamera()).Position();
    }

    auto view = scene.GetRegistry().view<TransformComponent, ParticleComponent>();
    for(auto [entity, trans, particle]: view.each()){
        /*if(particle.drawData == nullptr){
            particle.drawData = InstancingBuffer::Create();
        }*/

        /*if(particle.particleSystem.CurState() != ParticleSystem::State::Running){
            particle.particleSystem.Play();
        }*/
        particle.particleSystem.Update(scene, entity, trans, camPos);
    }
}

}
