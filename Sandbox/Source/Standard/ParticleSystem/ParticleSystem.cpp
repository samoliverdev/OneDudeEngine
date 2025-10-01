#include "ParticleSystem.h"
#include "Standard/Ultis/Ultis.h"
#include <OD/Core/Time.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/Model.h>
#include <OD/Core/ImGui.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp> // glm::linearRand, glm::sphericalRand
#include <random>

namespace Standard{

void SpawnModule::OnGui(){
    if(ImGui::CollapsingHeader("Spawn")){
        ImGui::DragInt("singleBustEmiterCount", &singleBustEmiterCount);
        ImGui::DragFloat("overTimeEmiterRate", &overTimeEmiterRate);
    }
}

void SpawnModule::OnStartSpawnUpdate(ParticleSystem& system){
    int singleNewParticles = singleBustEmiterCount;
    for(int i = 0; i < singleNewParticles; i++){
        system.SpawnNewParticle();
    }
}

void SpawnModule::OnSpawnUpdate(ParticleSystem& system){
    int newParticles = (int)(Time::DeltaTime() * overTimeEmiterRate);
    for(int i = 0; i < newParticles; i++){
        system.SpawnNewParticle();
    }
}

void InitialLifeModule::OnGui(){
    if(ImGui::CollapsingHeader("InitialLife")){
        ImGui::DragFloat("minLife", &minLife);
        ImGui::DragFloat("maxLife", &maxLife);
    }
}

void InitialLifeModule::OnInitParticle(ParticleData& particle){
    particle.life = Ultis::RandomRange(minLife, maxLife); 
}

void InitialVelocityModule::OnGui(){
    if(ImGui::CollapsingHeader("InitialVelocity")){
        ImGui::DragFloat3("minVelocity", &minVelocity.x);
        ImGui::DragFloat3("maxVelocity", &maxVelocity.x);
    }
}

void InitialVelocityModule::OnInitParticle(ParticleData& particle){
    particle.vel = Vector3(
        Ultis::RandomRange(minVelocity.x, maxVelocity.x), 
        Ultis::RandomRange(minVelocity.y, maxVelocity.y), 
        Ultis::RandomRange(minVelocity.z, maxVelocity.z) 
    ); 
}

void InitialSizeModule::OnGui(){
    if(ImGui::CollapsingHeader("InitialSize")){
        ImGui::DragFloat3("minSize", &minSize.x);
        ImGui::DragFloat3("maxSize", &maxSize.x);
    }
}

void InitialSizeModule::OnInitParticle(ParticleData& particle){
    particle.size = Vector3(
        Ultis::RandomRange(minSize.x, maxSize.x), 
        Ultis::RandomRange(minSize.y, maxSize.y), 
        Ultis::RandomRange(minSize.z, maxSize.z) 
    ); 
}

void InitialColorModule::OnGui(){
    if(ImGui::CollapsingHeader("InitialColor")){
        ImGui::ColorEdit4("minSize", &color.r);
    }
}

void InitialColorModule::OnInitParticle(ParticleData& particle){
    particle.color = color;
}

void UpdaterModule::OnGui(){
    if(ImGui::CollapsingHeader("Updater")){
        ImGui::DragFloat("gravityModifier", &gravityModifier);
    }
}

void UpdaterModule::OnParticleUpdate(ParticleData& particle, ParticleRunningData& runningData){
    particle.vel += Vector3(0.0f,-9.81f, 0.0f) * (runningData.delta * gravityModifier);
    particle.pos += particle.vel * runningData.delta;
}

void SizeOverLifetimeModule::OnGui(){
    if(ImGui::CollapsingHeader("SizeOverLifeTime")){
        ImGui::Checkbox("enable", &enable);
        ImGui::DragFloat("maxSize", &maxSize);
    }
}

void SizeOverLifetimeModule::OnParticleUpdate(ParticleData& p, ParticleRunningData& runningData){
    if(enable == false) return;
    p.size = math::mix(p.startSize, Vector3(maxSize), runningData.lifetime);
}

void ColorOverLifetimeModule::OnGui(){
    if(ImGui::CollapsingHeader("ColorOverLifetimeModule")){
        ImGui::Checkbox("enable", &enable);
        ImGui::ColorEdit4("colorA", &colorA, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
        ImGui::ColorEdit4("colorB", &colorB, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
    }
}

void ColorOverLifetimeModule::OnParticleUpdate(ParticleData& p, ParticleRunningData& runningData){
    if(enable == false) return;
    p.color = Color::Lerp(colorA, colorB, runningData.lifetime);
}

//////////////////////////////////////////////////////////

void ParticleSystem::OnGui(){
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
    initialSizeModule.OnGui();
    initialColorModule.OnGui();

    updaterModule.OnGui();
    sizeOverLifetimeModule.OnGui();
    colorOverLifetimeModule.OnGui();
}

void ParticleSystem::BindModules(){
    spawnModules.clear();
    initParticleModules.clear();
    updateModules.clear();

    spawnModules.push_back(&spawnModule);
    
    initParticleModules.push_back(&initialLifeModule);
    initParticleModules.push_back(&initialVelocityModule);
    initParticleModules.push_back(&initialSizeModule);
    initParticleModules.push_back(&initialColorModule);

    updateModules.push_back(&updaterModule);
    updateModules.push_back(&sizeOverLifetimeModule);
    updateModules.push_back(&colorOverLifetimeModule);
}

void ParticleSystem::Reset(){
    curDelayTime = delay;
    hasStarted = false;

    freeParticles.clear();
    for(int i = 0; i < particles.size(); i++){
        particles[i].life = 0;
    }
}

void ParticleSystem::Play(){
    state = State::Running;
    Reset();
}

void ParticleSystem::Stop(){
    state = State::Stop;
}

ParticleSystem::State ParticleSystem::CurState(){
    return state;
}

void ParticleSystem::SetMaxParticle(int inmaxParticles){
    maxParticles = inmaxParticles;
    particles.resize(maxParticles);
}

int ParticleSystem::GetNewParticle(){
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

void ParticleSystem::FreeParticle(int index){
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

void ParticleSystem::SpawnNewParticle(){
    int newIndex = GetNewParticle();
    if(newIndex == -1) return;

    ParticleData& p = particles[newIndex];
    for(auto* i: initParticleModules) i->OnInitParticle(p);
    p.startSize = p.size;
    p.startLife = p.life;
    if(simulationSpace == SimulationSpace::WorldSpace){
        p.pos += currentGlobalTrans.Position();
    }
}

void ParticleSystem::Update(TransformComponent& trans, Vector3 camPos){
    if(state != State::Running) return;

    currentGlobalTrans = trans.ToTransform();

    if(spawnModules.size() == 0) BindModules();
    if(particles.size() != maxParticles) SetMaxParticle(maxParticles);

    float delta = Time::DeltaTime();

    curDelayTime -= delta;
    if(curDelayTime > 0) return;

    if(hasStarted = false){
        hasStarted = true;
        for(auto* i: spawnModules) i->OnStartSpawnUpdate(*this);
    }
    for(auto* i: spawnModules) i->OnSpawnUpdate(*this);

    particlesCount = 0;
    for(int i = 0; i < particles.size(); i++){
        ParticleData& p = particles[i]; // shortcut
        ParticleRunningData runningData;
        runningData.delta = delta;
        runningData.lifetime = math::clamp<float>(1.0f - (p.life / p.startLife), 0, 1);

        if(p.life > 0.0f){
            p.life -= delta;

            if(p.life > 0.0f){
                for(auto* i: updateModules) i->OnParticleUpdate(p, runningData);
                p.cameradistance = math::length2(p.pos - camPos);
                particlesCount++;
            }else{
                p.cameradistance = -1.0f;
                FreeParticle(i);
            }
        }
    }
}

void ParticleSystem::Sort(){
    std::sort(particles.begin(), particles.end());
    //std::sort(&particles[0], &particles[particles.size()]);
}

void ParticleSystem::SubmitDrawData(InstancingBuffer& buffer, const Matrix4& root){
    drawData.resize(particlesCount);
    int i = 0;
    for(int _i = 0; _i < particles.size(); _i++){
        if(particles[_i].life <= 0) continue;

        Transform t(particles[_i].pos, QuaternionIdentity, particles[_i].size);
        
        if(simulationSpace == SimulationSpace::Local){
            auto m = root * t.GetModelMatrix();
            drawData[i] = Matrix4(math::row(m, 0), math::row(m, 1), math::row(m, 2), (Vector4)particles[_i].color);
        } else {
            auto m = t.GetModelMatrix();
            drawData[i] = Matrix4(math::row(m, 0), math::row(m, 1), math::row(m, 2), (Vector4)particles[_i].color);
        }

        i += 1;
    }
    buffer.SetData(drawData.data(), particlesCount);
}

ParticleRendererFeature::ParticleRendererFeature(){
    material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Standard/Shaders/LitParticle.glsl"));
    material->SetEnableInstancing(true);
    material->SetFloat("smoothness", 0);
    mesh = CreateRef<Model>();
    Model::CreateFromFile(*mesh, "Engine/Models/Cube.obj", {nullptr, 1, false});
}

void ParticleRendererFeature::OnCollectRenderData(const Camera& cam, std::vector<RenderData>& outRenderData){
    auto view = scene->GetRegistry().view<TransformComponent, ParticleComponent>();
    for(auto [entity, trans, particle]: view.each()){

        particle.particleSystem.SubmitDrawData(*particle.drawData, trans.GlobalModelMatrix());
        if(particle.drawData->Count() == 0) continue;

        RenderData renderData;
        renderData.distance = math::distance(trans.Position(), cam.viewPos);
        renderData.aabb = AABB(trans.Position(), {10, 10, 10});
        renderData.targetMaterial = material.get();
        renderData.targetMesh = mesh->meshs[0].get();
        renderData.targetMatrix = trans.GlobalModelMatrix();
        renderData.instancingBuffer = particle.drawData.get();
        renderData.customShadowPass = material->DepthPass() != -1 ? renderData.targetMaterial : nullptr;
        outRenderData.push_back(renderData);
    }
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
    auto view = scene.GetRegistry().view<TransformComponent, ParticleComponent>();
    for(auto [entity, trans, particle]: view.each()){
        if(particle.drawData == nullptr){
            particle.drawData = CreateRef<InstancingBuffer>();
        }

        if(particle.particleSystem.CurState() != ParticleSystem::State::Running){
            particle.particleSystem.Play();
        }
        particle.particleSystem.Update(trans, Vector3Zero);
    }
}

}