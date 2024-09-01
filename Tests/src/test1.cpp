#include <gtest/gtest.h>
#include <OD/OD.h>
#include <stdio.h>
#include <filesystem>

TEST(MultiplyTests, TestIntegerOne_One){
    const auto expected = 1;
    const auto actual = 1;
    ASSERT_EQ(expected, actual);
}

TEST(MultiplyTests, TestIntegerOne_One2){
    const auto expected = 1;
    const auto actual = 0;
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
    std::filesystem::current_path("../");

    OD::Platform::ShowWindow(false);

    if(!OD::Application::Create(CreateMainModule(), GetStartAppConfig())) {
        printf("Application failed to create!.\n");
        return 1;
    }

    ::testing::InitGoogleTest(&argc, argv);
    int r = RUN_ALL_TESTS();

    OD::Application::Exit();

    printf("Tests\n");

    return r;
}