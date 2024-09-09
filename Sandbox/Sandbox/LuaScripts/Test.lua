local text = "Script1"

function OnStart()
    LogInfo("Lua OnStart")
    LogInfo(entity:GetInfoComponent().name)
    entity:AddComponent(LightComponent())
end

function OnDestroy()
    LogInfo("Lua OnDestroy")
end

function OnUpdate()
    local info = entity:GetComponent(InfoComponent())
    info.name = "Lolo"

    LogInfo("Lua OnUpdate")

    --if entity:HasComponent(LightComponent()) then
    --    entity:RemoveComponent(LightComponent())
    --end
end