local text = "Script1"

function OnStart()
    LogInfo("Lua OnStart")
    LogInfo(scene:GetInfoComponent(entity).name) --LogInfo(entity:GetInfoComponent().name)
    scene:AddComponent(entity, LightComponent()) --entity:AddComponent(LightComponent())
    
    local transform = scene:GetComponent(entity, TransformComponent()) --entity:GetComponent(TransformComponent())
    transform:LocalScale(Vector3(10, 20, 5))
end

function OnDestroy()
    LogInfo("Lua OnDestroy")
end

function OnUpdate()
    local info = scene:GetComponent(entity, InfoComponent()) --entity:GetComponent(InfoComponent())
    info.name = "Lolo"

    --LogInfo("Lua OnUpdate")

    --if entity:HasComponent(LightComponent()) then
    --    entity:RemoveComponent(LightComponent())
    --end
end