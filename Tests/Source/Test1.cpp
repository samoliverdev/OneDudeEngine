#include <gtest/gtest.h>
#include <OD/Base.h>
#include <OD/Core/Module.h>
#include <OD/Core/Application.h>
#include <OD/Core/Project.h>
#include <stdio.h>
#include <filesystem>

TEST(MultiplyTests, TestIntegerOne_One){
    const auto expected = 1;
    const auto actual = 1;
    ASSERT_EQ(expected, actual);
}

TEST(MultiplyTests, TestIntegerOne_One2){
    const auto expected = 1;
    const auto actual = 1;
    ASSERT_EQ(expected, actual);
}

class EmptyModule: public OD::Module {
    void OnInit() override{};
    void OnUpdate(float deltaTime) override{};
    void OnRender(float deltaTime) override{};
    void OnGUI() override{};
    void OnResize(int width, int height) override{};
};  

OD::ApplicationConfig GetStartAppConfig(){
    return OD::ApplicationConfig{ 0, 0,800, 600, "Tests"};
}

OD::Module* CreateMainModule(){
    return new EmptyModule();
}

int main(int argc, char** argv){
    ::testing::InitGoogleTest(&argc, argv);

    // IMPORTANT: skip engine boot when CMake is discovering tests
    if(::testing::GTEST_FLAG(list_tests) ||
        ::testing::GTEST_FLAG(filter).find("GfxCommandDispatch.") != std::string::npos ||
        ::testing::GTEST_FLAG(filter).find("TerrainSystemDataTest.") != std::string::npos ||
        ::testing::GTEST_FLAG(filter).find("RenderDataCullingTest.") != std::string::npos ||
        ::testing::GTEST_FLAG(filter).find("StandingRagdollTest.") != std::string::npos ||
        ::testing::GTEST_FLAG(filter).find("StandingRagdollStrengthTest.") != std::string::npos ||
        ::testing::GTEST_FLAG(filter).find("RagdollPoseBlendTest.") != std::string::npos ||
        ::testing::GTEST_FLAG(filter).find("RagdollInPlacePoseTest.") != std::string::npos){
        return RUN_ALL_TESTS();
    }

    printf("Booting engine for tests...\n");

    bool terrainGraphics=::testing::GTEST_FLAG(filter).find("TerrainSystemRenderTest.") != std::string::npos;
    std::string testProjectPath=terrainGraphics?TERRAIN_TEST_PROJECT_PATH:RESOURCES_PATH;
    if(terrainGraphics) std::filesystem::current_path(testProjectPath);
    auto customProject=terrainGraphics?OD::CreateRef<OD::Project>():nullptr;
    if(!OD::Application::Create(CreateMainModule(), GetStartAppConfig(), testProjectPath.c_str(),{},customProject)){
        printf("Application failed to create!.\n");
        return 1;
    }

    int result = RUN_ALL_TESTS();

    // Application::Exit terminates with zero; preserve a failing test result for CTest.
    if(result != 0) return result;

    // optional cleanup
    OD::Application::Exit();

    return result;
}
