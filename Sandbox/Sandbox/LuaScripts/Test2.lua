local text = "Script2"

function OnStart()
    LogInfo("Lua OnStart 2")
    LogInfo(entity:GetInfoComponent().name)

    transform = entity:GetTransformComponent()
    transform:SetLocalPosition(Vector3(0, 10, 20))
end

function OnDestroy()
    LogInfo("Lua OnDestroy 2")
end

function OnUpdate()
    LogInfo("Lua OnUpdate 2")
end