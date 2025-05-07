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

local Script = {
    elapsed = 0.0,
    tt = 2,
    ff = true,
    text = "Script1",
    text2 = "lolo2",
    test = Vector3(1, 2, 3)
}

function Script:OnStart()
    LogInfo("Lua OnStart")
    LogInfo(scene:GetInfoComponent(entity).name) --LogInfo(entity:GetInfoComponent().name)
    scene:AddComponent(entity, LightComponent()) --entity:AddComponent(LightComponent())
    
    local transform = scene:GetComponent(entity, TransformComponent()) --entity:GetComponent(TransformComponent())
    transform:LocalScale(Vector3(10, 20, 5))
end

function Script:OnDestroy()
    LogInfo("Lua OnDestroy")
end

function Script:OnUpdate()
    local info = scene:GetComponent(entity, InfoComponent()) --entity:GetComponent(InfoComponent())
    info.name = self.text2
end

return Script