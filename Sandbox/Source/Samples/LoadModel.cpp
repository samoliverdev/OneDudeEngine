#include "LoadModel.h"
#include "Ultis/Ultis.h"
#include <string>
#include <regex>
#include <stdexcept>

std::pair<std::string, std::string> splitGLSLCode(const std::string& combinedCode) {
    std::string vertexShader;
    std::string fragmentShader;
    
    // Find vertex and fragment function positions
    size_t vertexStart = combinedCode.find("void Vertex()");
    size_t fragmentStart = combinedCode.find("void Fragment()");
    
    // Basic error checking
    if(vertexStart == std::string::npos || fragmentStart == std::string::npos){
        return {"// Error: Missing shader function", "// Error: Missing shader function"};
    }
    
    if(vertexStart > fragmentStart) {
        return {"// Error: Vertex shader must come before Fragment shader",
                "// Error: Vertex shader must come before Fragment shader"};
    }
    
    // Extract common code (uniforms and helper functions) before Vertex()
    std::string commonCode = "#version 330 core\n";
    if(vertexStart > 0){
        commonCode += combinedCode.substr(0, vertexStart);
    }
    
    vertexShader = commonCode + combinedCode.substr(vertexStart, fragmentStart - vertexStart);// Extract vertex shader code
    fragmentShader = commonCode + combinedCode.substr(fragmentStart);// Extract fragment shader code
    return {vertexShader, fragmentShader};
}

std::vector<std::pair<std::string, std::string>> extractPassBlocks(const std::string& shaderCode) {
    std::vector<std::pair<std::string, std::string>> passes;
    
    // Regex to match BeginPass/EndPass blocks
    std::regex passRegex(R"(BeginPass\s*\(\s*(\w+)\s*\)\s*([\s\S]*?)\s*EndPass\s*\(\s*\))");
    auto passes_begin = std::sregex_iterator(shaderCode.begin(), shaderCode.end(), passRegex);
    auto passes_end = std::sregex_iterator();
    
    for (std::sregex_iterator i = passes_begin; i != passes_end; ++i) {
        std::smatch match = *i;
        std::string passName = match[1];
        std::string passContent = "#version 330 core\n" + match[2].str();
        
        // Clean up leading and trailing whitespace
        while (!passContent.empty() && std::isspace(passContent.front())) {
            passContent.erase(0, 1);
        }
        while (!passContent.empty() && std::isspace(passContent.back())) {
            passContent.pop_back();
        }
        
        passes.emplace_back(passName, passContent);
    }
    
    if (passes.empty()) {
        // If no passes found, treat entire code as unnamed pass if it contains content
        std::string trimmedCode = shaderCode;
        while (!trimmedCode.empty() && std::isspace(trimmedCode.front())) {
            trimmedCode.erase(0, 1);
        }
        while (!trimmedCode.empty() && std::isspace(trimmedCode.back())) {
            trimmedCode.pop_back();
        }
        if (!trimmedCode.empty()) {
            passes.emplace_back("Unnamed", "#version 330 core\n" + trimmedCode);
        } else {
            passes.emplace_back("Unnamed", "#version 330 core\n// No valid Pass blocks found");
        }
    }
    
    return passes;
}

std::vector<std::pair<std::string, std::string>> extractPassBlocks2(const std::string& shaderCode) {
    std::vector<std::pair<std::string, std::string>> passes;
    
    // First, extract content outside of pass blocks
    std::string commonCode;
    std::regex passRegex(R"(BeginPass\s*\(\s*(\w+)\s*\)\s*([\s\S]*?)\s*EndPass\s*\(\s*\))");
    
    size_t lastPos = 0;
    auto passes_begin = std::sregex_iterator(shaderCode.begin(), shaderCode.end(), passRegex);
    auto passes_end = std::sregex_iterator();
    
    // Collect common code before first pass and between passes
    for (std::sregex_iterator i = passes_begin; i != passes_end; ++i) {
        std::smatch match = *i;
        size_t matchPos = match.position();
        
        // Add content before this pass to commonCode
        if (matchPos > lastPos) {
            commonCode += shaderCode.substr(lastPos, matchPos - lastPos);
        }
        
        lastPos = matchPos + match.length();
    }
    
    // Add any remaining content after last pass
    if (lastPos < shaderCode.length()) {
        commonCode += shaderCode.substr(lastPos);
    }
    
    // Clean up commonCode whitespace
    while (!commonCode.empty() && std::isspace(commonCode.front())) {
        commonCode.erase(0, 1);
    }
    while (!commonCode.empty() && std::isspace(commonCode.back())) {
        commonCode.pop_back();
    }
    if (!commonCode.empty()) {
        commonCode += "\n\n";  // Add spacing between common code and pass content
    }
    
    // Now extract pass blocks and include common code
    for (std::sregex_iterator i = passes_begin; i != passes_end; ++i) {
        std::smatch match = *i;
        std::string passName = match[1];
        std::string passContent = match[2].str();
        
        // Clean up pass content whitespace
        while (!passContent.empty() && std::isspace(passContent.front())) {
            passContent.erase(0, 1);
        }
        while (!passContent.empty() && std::isspace(passContent.back())) {
            passContent.pop_back();
        }
        
        // Combine common code with pass-specific content
        std::string fullContent = "#version 330 core\n" + commonCode + passContent;
        passes.emplace_back(passName, fullContent);
    }
    
    if (passes.empty()) {
        std::string trimmedCode = shaderCode;
        while (!trimmedCode.empty() && std::isspace(trimmedCode.front())) {
            trimmedCode.erase(0, 1);
        }
        while (!trimmedCode.empty() && std::isspace(trimmedCode.back())) {
            trimmedCode.pop_back();
        }
        passes.emplace_back("Unnamed", "#version 330 core\n" + trimmedCode);
    }
    
    return passes;
}

void LoadModelSample::OnInit(){
    LogInfo("Game Init");

    Application::Vsync(false);

    camTransform.LocalPosition(Vector3(0, 10, 70));
    camTransform.LocalEulerAngles(Vector3(0, 0, 0));
    camMove.transform = &camTransform;

    model = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/cube.gltf");
    model->materials[0]->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Unlit.glsl"));
    model->materials[0]->SetTexture("mainTex", AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/Rock.jpg"));
    model->materials[0]->SetVector4("color", {1,1,1,1});

    for(int i = 0; i < 100000; i++){
        float posRange = 25;
        Transform t;

        float angle = 20.0f * i; 
        t.LocalPosition(Vector3(random(-posRange, posRange), random(0, posRange), random(-posRange, posRange)));
        t.LocalEulerAngles(Vector3(random(-180, 180), random(-180, 180), random(-180, 180)));
        transforms.push_back(t.GetLocalModelMatrix());
    }

    useInstancing = true;
    return;
    
    std::string combined = R"(
        #include "Base.glsl"
        uniform vec3 color;
        vec3 GetColor(){ return color; }
        void Vertex() {
            gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        }
        void Fragment() {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    auto [vertexShader, fragmentShader] = splitGLSLCode(combined);
    std::cout << "Vertex Shader:\n" << vertexShader << "\n\n";
    std::cout << "Fragment Shader:\n" << fragmentShader << "\n";

    std::string shaderCode = R"(
        float Test(){
            return 10;
        } 

        BeginPass(Main)
            uniform vec3 color;
            vec3 GetColor(){ return color; }
            void Vertex() {
                gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
            }
            void Fragment() {
                gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
            }
        EndPass()
        BeginPass(Other)
            void Vertex() {
                gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
            }
        EndPass()
    )";
    
    auto passes = extractPassBlocks2(shaderCode);
    for (const auto& [name, content] : passes) {
        std::cout << "Pass: " << name << "\n";
        std::cout << "Content:\n" << content << "\n\n";
    }
}

void LoadModelSample::OnUpdate(float deltaTime){
    camMove.OnUpdate();
    //modelTransform.localEulerAngles(Vector3(0, Platform::GetTime() * 20, 0));
}   

void LoadModelSample::OnRender(float deltaTime){
    OD_PROFILE_SCOPE("LoadModel_2::OnRender");

    cam.SetPerspective(60, 0.1f, 1000.0f, Application::ScreenWidth(), Application::ScreenHeight());
    cam.view = math::inverse(camTransform.GetLocalModelMatrix());

    Graphics::Begin();
    Graphics::Clean(0.1f, 0.1f, 0.1f, 1);
    Graphics::SetCamera(cam);

    Graphics::BeginRenderToScreen();
    if(useInstancing){  
        //model->materials[0]->SetEnableInstancing(true);
        model->materials[0]->DisableKeyword("SKINNED");
        model->materials[0]->EnableKeyword("INSTANCING");
        Graphics::DrawMeshInstancing(*model->meshs[0], *model->materials[0], &transforms[0], transforms.size());
    } else {
        model->materials[0]->DisableKeyword("SKINNED");
        model->materials[0]->DisableKeyword("INSTANCING");
        for(auto i: transforms){
            Graphics::DrawMesh(*model->meshs[0], *model->materials[0], i);
        }
    }

    //Graphics::DrawImgui(OnGUI);
    Graphics::EndRenderToScreen();
    
    Graphics::End();
}

void LoadModelSample::OnGUI(){
    //static bool show;
    //ImGui::ShowDemoWindow(&show);

    ImGui::Begin("Load Model Test");

    ImGui::Checkbox("Use Instancing", &useInstancing);
    ImGui::Spacing();
    
    ImGui::Text("DrawCalls: %d", Graphics::GetStats().drawCalls);
    //ImGui::Text("Vertices: %dk", Graphics::GetVerticesCount() / 1000);
    //ImGui::Text("Tris: %dk", Graphics::GetTrisCount() / 1000);
    
    if(Graphics::GetStats().vertices >= 1000000){
        ImGui::Text("Vertices: %.1fM", Graphics::GetStats().vertices / 1000000.0f);
    } else if(Graphics::GetStats().vertices >= 1000){
        ImGui::Text("Vertices: %.1fk", Graphics::GetStats().vertices / 1000.0f);
    } else {
        ImGui::Text("Vertices: %d", Graphics::GetStats().vertices);
    }

    if(Graphics::GetStats().tris >= 1000000){
        ImGui::Text("Tris: %.1fM", Graphics::GetStats().tris / 1000000.0f);
    } else if(Graphics::GetStats().tris >= 1000){
        ImGui::Text("Tris: %.1fk", Graphics::GetStats().tris / 1000.0f);
    } else {
        ImGui::Text("Tris: %d", Graphics::GetStats().tris);
    }
    
    ImGui::End();
}

void LoadModelSample::OnResize(int width, int height){}
void LoadModelSample::OnExit(){}