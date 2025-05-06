function OnStart()
    print("Lua OnStart 2", text)
    LogInfo(scene:GetInfoComponent(entity).name) --LogInfo(entity:GetInfoComponent().name)

    local transform = scene:GetTransformComponent(entity) --entity:GetTransformComponent()
    transform:LocalPosition(Vector3(0, 10, 20))

    Application.Vsync(false)

    --local invalidEntity = EntityNull();
    --print(scene:IsValid(invalidEntity));
    
    local fromLua = scene:AddEntity("From Lua") --entity:GetScene():AddEntity("From Lua")
    local light = scene:AddComponent(fromLua, LightComponent()) --fromLua:AddComponent(LightComponent())
    light.color = Color(0, 0, 1, 0)
    light.type = LightComponentType.Point
    light.renderShadow = true
    local lightT =  scene:GetComponent(fromLua, TransformComponent()) --fromLua:GetComponent(TransformComponent())
    lightT:LocalPosition(Vector3(0, 1, 0))

    --local tex = Texture2D.CreateFromFile("Engine/Textures/White.jpg", Texture2DSetting())
    --local tex = Texture2D.New("Engine/Textures/White.jpg", Texture2DSetting())
    local tex = Texture2D("Engine/Textures/White.jpg", Texture2DSetting())
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

local Script = {
    elapsed = 0,
    text = "Script1",
    text2 = "lolo2",
    v = Vector3(1, 2, 3),
    arr = {1,2,3},
    customData = { a = 20, b = "Custom"}
}

function Script:OnStart()
    print("Lua OnStart 2", self.text)
    LogInfo(scene:GetInfoComponent(entity).name) --LogInfo(entity:GetInfoComponent().name)

    local transform = scene:GetTransformComponent(entity) --entity:GetTransformComponent()
    transform:LocalPosition(Vector3(0, 10, 20))

    Application.Vsync(false)

    --local invalidEntity = EntityNull();
    --print(scene:IsValid(invalidEntity));
    
    local fromLua = scene:AddEntity("From Lua") --entity:GetScene():AddEntity("From Lua")
    local light = scene:AddComponent(fromLua, LightComponent()) --fromLua:AddComponent(LightComponent())
    light.color = Color(0, 0, 1, 0)
    light.type = LightComponentType.Point
    light.renderShadow = true
    local lightT = scene:GetComponent(fromLua, TransformComponent()) --fromLua:GetComponent(TransformComponent())
    lightT:LocalPosition(Vector3(0, 1, 0))

    --local tex = Texture2D.CreateFromFile("Engine/Textures/White.jpg", Texture2DSetting())
    --local tex = Texture2D.New("Engine/Textures/White.jpg", Texture2DSetting())
    --local tex = Texture2D("Engine/Textures/White.jpg", Texture2DSetting())
    --print(tex:Width(), tex:Height())
end

function Script:OnDestroy()
    LogInfo("Lua OnDestroy 2")
end

function Script:OnUpdate()
    if Input.IsKeyDown(KeyCode.Y) then
        LogInfo("press key Y")
    end
end

return Script