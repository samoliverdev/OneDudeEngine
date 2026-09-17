#include <gtest/gtest.h>
#include <OD/Gfx/GfxReflection.h>
#include <OD/Gfx/Gfx.h>
#include <array>

using namespace OD;

TEST(GfxImGui, RecordsSnapshotOwnershipInRenderFrame)
{
    Gfx::RenderFrame frame;
    void* snapshot = reinterpret_cast<void*>(static_cast<uintptr_t>(0x1234));

    frame.RecordImGuiDrawData(snapshot, nullptr);

    ASSERT_EQ(frame.renderCommands.commands.size(), 1u);
    EXPECT_EQ(
        frame.renderCommands.commands.front().type,
        Gfx::CommandBuffer::Type::RenderImGui
    );
    EXPECT_EQ(frame.renderCommands.commands.front().renderImGui.snapshotIndex, 0u);
    EXPECT_EQ(frame.imguiSnapshots.size(), 1u);
    EXPECT_EQ(frame.imguiSnapshots.front().data, snapshot);
}

TEST(GfxImGui, InsertsRenderBeforeFramebufferEnd)
{
    Gfx::RenderFrame frame;
    frame.renderCommands.BeginWindowFramebuffer();
    frame.renderCommands.EndFramebuffer();

    frame.RecordImGuiDrawData(reinterpret_cast<void*>(static_cast<uintptr_t>(0x5678)), nullptr);

    ASSERT_EQ(frame.renderCommands.commands.size(), 3u);
    EXPECT_EQ(frame.renderCommands.commands[1].type, Gfx::CommandBuffer::Type::RenderImGui);
    EXPECT_EQ(frame.renderCommands.commands[2].type, Gfx::CommandBuffer::Type::EndFramebuffer);
}

TEST(GfxReflection, ReflectsShaderVariableTypes)
{
    const char* shader = R"(
        struct Nested {
            float value;
        };

        layout(std140, set = 0, binding = 0) uniform Main {
            int intValue;
            float floatValue;
            vec2 vector2Value;
            vec3 vector3Value;
            vec4 vector4Value;
            mat4 matrix4Value;
            float floatListValue[2];
            vec4 vector4ListValue[2];
            mat4 matrix4ListValue[2];
            Nested bufferValue;
        };

        #ifdef VERTEX
        void main(){
        }
        #endif

        #ifdef FRAGMENT
        void main(){
        }
        #endif
    )";

    Gfx::ShaderReflection reflection;
    Gfx::Reflect(shader, reflection);

    ASSERT_EQ(reflection.bindings.size(), 1u);
    const auto& variables = reflection.bindings.front().variables;
    ASSERT_EQ(variables.size(), 10u);

    EXPECT_EQ(variables[0].type, Gfx::ShaderVariable::Type::Int);
    EXPECT_EQ(variables[1].type, Gfx::ShaderVariable::Type::Float);
    EXPECT_EQ(variables[2].type, Gfx::ShaderVariable::Type::Vector2);
    EXPECT_EQ(variables[3].type, Gfx::ShaderVariable::Type::Vector3);
    EXPECT_EQ(variables[4].type, Gfx::ShaderVariable::Type::Vector4);
    EXPECT_EQ(variables[5].type, Gfx::ShaderVariable::Type::Matrix4);
    EXPECT_EQ(variables[6].type, Gfx::ShaderVariable::Type::FloatList);
    EXPECT_EQ(variables[7].type, Gfx::ShaderVariable::Type::Vector4List);
    EXPECT_EQ(variables[8].type, Gfx::ShaderVariable::Type::Matrix4List);
    EXPECT_EQ(variables[9].type, Gfx::ShaderVariable::Type::Buffer);
}

TEST(GfxFramebuffer, RecordsAttachmentLayerAndMip)
{
    Gfx::CommandBuffer commands;

    commands.BeginFramebuffer(42, 5, 3);

    ASSERT_EQ(commands.commands.size(), 1u);
    const auto& begin = commands.commands.front();
    EXPECT_EQ(begin.type, Gfx::CommandBuffer::Type::BeginFramebuffer);
    EXPECT_EQ(begin.beginFramebuffer.framebuffer, 42u);
    EXPECT_EQ(begin.beginFramebuffer.layer, 5u);
    EXPECT_EQ(begin.beginFramebuffer.mip, 3u);
}

TEST(GfxFramebuffer, TestsClearFlags)
{
    EXPECT_TRUE(Gfx::HasFlag(Gfx::ClearFlags::Color | Gfx::ClearFlags::Depth, Gfx::ClearFlags::Depth));
    EXPECT_FALSE(Gfx::HasFlag(Gfx::ClearFlags::Color, Gfx::ClearFlags::Stencil));
}

TEST(GfxCubemap, RecordsUploadCommand)
{
    Gfx::ResourceCommands commands;
    const std::array<uint8_t, 24> pixels{};
    commands.UploadCubemap(7, pixels.data(), pixels.size());

    ASSERT_EQ(commands.commands.size(), 1u);
    EXPECT_EQ(commands.commands.front().type, Gfx::ResourceCommands::Type::UploadCubemap);
    EXPECT_EQ(commands.commands.front().uploadCubemap.id, 7u);
    EXPECT_EQ(commands.commands.front().uploadCubemap.size, pixels.size());
    EXPECT_NE(commands.commands.front().uploadCubemap.data, pixels.data());
}
