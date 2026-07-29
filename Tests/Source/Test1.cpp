#include <gtest/gtest.h>
#include <OD/Base.h>
#include <OD/Core/Module.h>
#include <OD/Core/Application.h>
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
    if(::testing::GTEST_FLAG(list_tests)){
        return RUN_ALL_TESTS();
    }

    printf("Booting engine for tests...\n");

    if(!OD::Application::Create(CreateMainModule(), GetStartAppConfig(), RESOURCES_PATH "")){
        printf("Application failed to create!.\n");
        return 1;
    }

    int result = RUN_ALL_TESTS();

    // optional cleanup
    OD::Application::Exit();

    return result;
}