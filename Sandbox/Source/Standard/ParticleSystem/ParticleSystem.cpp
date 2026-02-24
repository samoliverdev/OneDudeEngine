#include "ParticleSystem.h"
#include "Standard/Ultis/Ultis.h"
#include <OD/Core/Time.h>
#include <OD/Graphics/Geometry.h>
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

void SpawnModule::OnStartSpawnUpdate(ParticleEmiter& system){
    int singleNewParticles = singleBustEmiterCount;
    for(int i = 0; i < singleNewParticles; i++){
        system.SpawnNewParticle();
    }
}

void SpawnModule::OnSpawnUpdate(ParticleEmiter& system){
    /*int newParticles = (int)(Time::DeltaTime() * overTimeEmiterRate);
    for(int i = 0; i < newParticles; i++){
        system.SpawnNewParticle();
    }*/

    emissionAccumulator += Time::DeltaTime() * overTimeEmiterRate;

    int newParticles = (int)emissionAccumulator;
    emissionAccumulator -= newParticles; // keep the leftover fraction

    for (int i = 0; i < newParticles; i++){
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

void InitialSizeModule::OnInitParticle(ParticleData& particle){
    if(uniforSize){
        particle.size = Vector3(Ultis::RandomRange(minSize.x, maxSize.x));
    } else {
        particle.size = Vector3(
            Ultis::RandomRange(minSize.x, maxSize.x), 
            Ultis::RandomRange(minSize.y, maxSize.y), 
            Ultis::RandomRange(minSize.z, maxSize.z) 
        ); 
    }
}

void InitialColorModule::OnGui(){
    if(ImGui::CollapsingHeader("InitialColor")){
        ImGui::ColorEdit4("color", &color.r, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
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
    initialSizeModule.OnGui();
    initialColorModule.OnGui();

    updaterModule.OnGui();
    sizeOverLifetimeModule.OnGui();
    colorOverLifetimeModule.OnGui();
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
    initParticleModules.push_back(&initialSizeModule);
    initParticleModules.push_back(&initialColorModule);

    updateModules.push_back(&updaterModule);
    updateModules.push_back(&sizeOverLifetimeModule);
    updateModules.push_back(&colorOverLifetimeModule);
    updateModules.push_back(&collisionPhysicModule);
}

void ParticleEmiter::Reset(){
    runningTime = 0;
    curDelayTime = delay;
    hasStarted = false;

    currentParticleIndex = 0;
    particlesCount = 0;
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

void ParticleEmiter::SpawnNewParticle(){
    int newIndex = GetNewParticle();
    if(newIndex == -1) return;

    ParticleData& p = particles[newIndex];
    for(auto* i: initParticleModules) i->OnInitParticle(p);
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

    float delta = Time::DeltaTime();
    curDelayTime -= delta;
    if(curDelayTime > 0) return;

    runningTime += delta;

    bool runSpwan = true;
    if(isLooping == false && runningTime > duration) runSpwan = false;

    if(runSpwan){
        if(hasStarted == false){
            hasStarted = true;
            for(auto* i: spawnModules) i->OnStartSpawnUpdate(*this);
        }
        for(auto* i: spawnModules) i->OnSpawnUpdate(*this);
    }

    particlesCount = 0;
    for(int i = 0; i < particles.size(); i++){
        ParticleData& p = particles[i]; // shortcut
        ParticleRunningData runningData;
        runningData.delta = delta;
        runningData.lifetime = math::clamp<float>(1.0f - (p.life / p.startLife), 0, 1);

        if(p.life > 0.0f){
            /*p.life -= delta;

            if(p.life > 0.0f){
                for(auto* i: updateModules) i->OnParticleUpdate(p, runningData);
                p.cameradistance = math::length2(p.pos - camPos);
                particlesCount++;
            }else{
                p.cameradistance = -1.0f;
                FreeParticle(i);
            }*/

            for(auto* i: updateModules) i->OnParticleUpdate(p, runningData);
            p.life -= delta;
            if(p.life > 0.0f){
                p.cameradistance = math::length2(p.pos - camPos);
                particlesCount++;
            }else{
                if(p.handleCollision) curParticleCollisionCount -= 1;
                p.cameradistance = -1.0f;
                FreeParticle(i);
            }
        }
    }

    if(isLooping == false && particlesCount <= 0){
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
    model[2] = glm::vec4(camForward, 0.0f);
    model[3] = glm::vec4(objectPos,  1.0f);

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
    glm::vec3 axisZ = glm::vec3(0, 0, 1); // billboard always faces camera

    // Local X axis → cross(Y, Z)
    glm::vec3 axisX = glm::normalize(glm::cross(axisY, axisZ));
    axisY = glm::normalize(glm::cross(axisZ, axisX));

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
            targetModelMatrix = MakeBillboard(t.Position(), cam->view, cam->projection) * glm::scale(glm::mat4(1.0f), t.Scale());
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
            drawData[i] = Matrix4(math::row(m, 0), math::row(m, 1), math::row(m, 2), (Vector4)particles[_i].color);
        } else {
            auto m = targetModelMatrix;
            drawData[i] = Matrix4(math::row(m, 0), math::row(m, 1), math::row(m, 2), (Vector4)particles[_i].color);
        }

        i += 1;
    }

    SortDrawDataByParticleDistance(drawData, particles);

    buffer.SetData(drawData.data(), particlesCount);
}

void ParticleSystem::OnGui(){
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

    if(ImGui::Button("PushEmiter")){
        Stop();
        emiters.push_back(ParticleEmiter());
    }

    if(ImGui::Button("PopEmiter")){
        Stop();
        if(emiters.size() > 1) emiters.pop_back();
    }

    ImGui::Separator();

    Assert(emiters.size() >= 1);
    auto runningTime = emiters[0].runningTime;

    auto state = ParticleEmiter::State::Stop;
    int particlesCount = 0;
    int particlesCollisionCount = 0;
    for(int i = 0; i < emiters.size(); i++){
        particlesCount += emiters[i].particlesCount;
        particlesCollisionCount += emiters[i].curParticleCollisionCount;
        if(emiters[i].state == ParticleEmiter::State::Running) state = ParticleEmiter::State::Running;
    }

    ImVec4 color = ImVec4(1.0f, 0.7f, 0.2f, 1.0f); // orange
    ImGui::TextColored(color, "State %s", std::string(magic_enum::enum_name(state)).c_str());
    ImGui::Text("RunTime: %f", runningTime);
    ImGui::Text("ParticleCounts: %d", particlesCount);
    ImGui::Text("ParticleCollisionCounts: %d", particlesCollisionCount);

    if(state == ParticleEmiter::State::Running && ImGui::Button("Stop")){
        Stop();
    } else if(state == ParticleEmiter::State::Stop && ImGui::Button("Play")){
        Play();
    }
}

bool ParticleSystem::IsPlaying(){
    for(auto& i: emiters){
        if(i.state == ParticleEmiter::State::Running) return true; 
    }
    return true;
}

void ParticleSystem::Play(){
    for(auto& i: emiters) i.Play();
}

void ParticleSystem::Stop(){
    for(auto& i: emiters) i.Stop();
}

void ParticleSystem::Update(Scene& scene, Entity e, TransformComponent& trans, Vector3 camPos){
    for(auto& i: emiters) i.Update(scene, e, trans, camPos);
}

ParticleRendererFeature::ParticleRendererFeature(){
    material = CreateRef<Material>(AssetManager::Get().LoadAsset<Shader>("Standard/Shaders/UnlitParticleBlend.glsl"));
    material->SetEnableInstancing(true);
    material->SetFloat("smoothness", 0);
    mesh = Asset::CreateFromFile<Model>("Engine/Models/Cube.obj", ModelLoadSettings{nullptr, 1, false}); //Model::CreateFromFile("Engine/Models/Cube.obj", {nullptr, 1, false});
}

void ParticleRendererFeature::OnCollectRenderData(const Camera& cam, std::vector<RenderData>& outRenderData){
    auto view = scene->GetRegistry().view<TransformComponent, ParticleComponent>();
    for(auto [entity, trans, particle]: view.each()){

        for(int i = 0; i < particle.particleSystem.emiters.size(); i++){
            auto& emiter = particle.particleSystem.emiters[i];
            //if(emiter.state != ParticleEmiter::State::Running) continue;

            emiter.SubmitDrawData(*emiter.dataBuffer, trans.GlobalModelMatrix(), &cam);
            if(emiter.dataBuffer->Count() == 0) continue;

            Assert(emiter.rendererModule.material != nullptr);

            RenderData renderData;
            renderData.distance = math::distance(trans.Position(), cam.viewPos);
            renderData.distance -= i * 0.01f; 
            renderData.targetMatrix = trans.GlobalModelMatrix();
            renderData.aabb = transform_aabb_optimized_abs_center_extents(AABB({0, 0, 0}, 10, 10, 10), renderData.targetMatrix);
            renderData.targetMaterial = emiter.rendererModule.material.get(); //material.get();
            renderData.targetMesh = emiter.rendererModule.model->meshs[0].get(); //mesh->meshs[0].get();
            renderData.instancingBuffer = emiter.dataBuffer.get();
            renderData.SetFlag(RenderData::Flag::RenderShadow, material->IsBlend() == false); //renderData.renderShadow = material->IsBlend() == false;
            renderData.customShadowPass = material->DepthPass() != -1 ? renderData.targetMaterial : nullptr;
            outRenderData.push_back(renderData);
            
        }
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
    Vector3 camPos = Vector3Zero; 
    if(scene.GetMainCamera() != EntityNull){
        camPos = scene.GetComponent<TransformComponent>(scene.GetMainCamera()).Position();
    }

    auto view = scene.GetRegistry().view<TransformComponent, ParticleComponent>();
    for(auto [entity, trans, particle]: view.each()){
        /*if(particle.drawData == nullptr){
            particle.drawData = CreateRef<InstancingBuffer>();
        }*/

        /*if(particle.particleSystem.CurState() != ParticleSystem::State::Running){
            particle.particleSystem.Play();
        }*/
        particle.particleSystem.Update(scene, entity, trans, camPos);
    }
}

}