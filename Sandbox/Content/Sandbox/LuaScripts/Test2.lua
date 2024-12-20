local text = "Script2"

function OnStart()
    print("Lua OnStart 2", text)
    LogInfo(entity:GetInfoComponent().name)

    local transform = entity:GetTransformComponent()
    transform:LocalPosition(Vector3(0, 10, 20))

    Application.Vsync(false)
    
    local fromLua = entity:GetScene():AddEntity("From Lua")
    local light = fromLua:AddComponent(LightComponent())
    light.color = Color(0, 0, 1, 0)
    light.type = LightComponentType.Point
    light.renderShadow = true
    local lightT = fromLua:GetComponent(TransformComponent())
    lightT:LocalPosition(Vector3(0, 1, 0))

    local tex = Texture2D.CreateFromFile("Engine/Textures/White.jpg", Texture2DSetting())
    print(tex:Width(), tex:Height())
end

function OnDestroy()
    LogInfo("Lua OnDestroy 2")
end

function OnUpdate()
    --LogInfo("Lua OnUpdate 2")
    --print(Application.Vsync())
    --print(Application.ScreenWidth(), Application.ScreenHeight())

    if Input.IsKeyDown(KeyCode.Y) then
        LogInfo("press key Y")
    end
end