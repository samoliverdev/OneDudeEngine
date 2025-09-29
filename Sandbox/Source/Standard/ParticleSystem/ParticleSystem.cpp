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

void ParticleSystem::OnGui(){
    if(ImGui::DragInt("maxParticles", &maxParticles)){
        SetMaxParticle(maxParticles);
        //Reset();
    }

    if(ImGui::DragFloat("delay", &delay)){
        //Reset();
    }

    if(ImGui::DragFloatRange2("startLifeTimeMinMax", &startLifeTimeMinMax.x, &startLifeTimeMinMax.y)){
        //Reset();
    }
    if(ImGui::DragFloatRange2("startSpeedTimeMinMax", &startSpeedTimeMinMax.x, &startSpeedTimeMinMax.y)){
        //Reset();
    }
    if(ImGui::DragFloatRange2("startSizeTimeMinMax", &startSizeTimeMinMax.x, &startSizeTimeMinMax.y)){
        //Reset();
    }

    if(ImGui::DragFloat("gravityModifier", &gravityModifier)){
        //Reset();
    }

    if(ImGui::DragInt("singleBustEmiterCount", &emiter.singleBustEmiterCount)){}
    if(ImGui::DragFloat("overTimeEmiterRate", &emiter.overTimeEmiterRate)){}

    if(ImGui::DragFloatRange2("minMaxVelX", &shape.minMaxVelX.x, &shape.minMaxVelX.y)){}
    if(ImGui::DragFloatRange2("minMaxVelY", &shape.minMaxVelY.x, &shape.minMaxVelY.y)){}
    if(ImGui::DragFloatRange2("minMaxVelZ", &shape.minMaxVelZ.x, &shape.minMaxVelZ.y)){}

    if(ImGui::Checkbox("minMaxVelZ", &sizeOverLifeTime.enable)){}
    if(ImGui::DragFloat("maxSize", &sizeOverLifeTime.maxSize)){}
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

    particles[newParticle] = Particle();
	return newParticle;
}

void ParticleSystem::FreeParticle(int index){
    freeParticles.push_back(index);
}

void ParticleSystem::SpawnParticle(Particle &particle){
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

    /*if(shape.type == ShapeType::Cone){
        auto dir = RandomDirectionInCone({0, 1, 0}, shape.coneAngle);
        particle.vel = math::normalizeSafe(dir) * Ultis::RandomRange(startSpeedTimeMinMax.x, startSpeedTimeMinMax.y); 
    }
    if(shape.type == ShapeType::Sphere){

    }*/
}

void ParticleSystem::Update(Vector3 camPos){
    if(state != State::Running) return;

    if(particles.size() != maxParticles) SetMaxParticle(maxParticles);

    float delta = Time::DeltaTime();

    curDelayTime -= delta;
    if(curDelayTime > 0) return;


    if(hasStarted = false){
        hasStarted = true;
        int singleNewParticles = emiter.singleBustEmiterCount;
        for(int i = 0; i < singleNewParticles; i++){
            int newIndex = GetNewParticle();
            if(newIndex == -1) continue;

            SpawnParticle(particles[newIndex]);
        }
    }

    int newParticles = (int)(delta * emiter.overTimeEmiterRate);
    LogInfo("newParticles: %d", newParticles);
    for(int i = 0; i < newParticles; i++){
        int newIndex = GetNewParticle();
        if(newIndex == -1) continue;

        SpawnParticle(particles[newIndex]);
    }

    particlesCount = 0;
    for(int i = 0; i < particles.size(); i++){
        Particle& p = particles[i]; // shortcut

        float t = math::clamp<float>(1.0f - (p.life / p.startLife), 0, 1);

        if(p.life > 0.0f){
            p.life -= delta;

            if(p.life > 0.0f){
                p.vel += Vector3(0.0f,-9.81f, 0.0f) * (delta * gravityModifier);
                p.pos += p.vel * delta;
                p.cameradistance = math::length2(p.pos - camPos);

                if(sizeOverLifeTime.enable){
                    p.size = math::mix(p.startSize, Vector3(sizeOverLifeTime.maxSize), t);
                }

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
        drawData[i] = root * t.GetModelMatrix();

        i += 1;
    }
    buffer.SetData(drawData.data(), particlesCount);
}

ParticleRendererFeature::ParticleRendererFeature(){
    material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));
    material->SetEnableInstancing(true);
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
        particle.particleSystem.Update(Vector3Zero);
    }
}

}