#include <gtest/gtest.h>
#include <OD/Gfx/ImGuiImages.h>
#include <OD/Gfx/Gfx.h>
#include <OD/Platform/BaseGpu/ImGuiImageResolver.h>

TEST(ImGuiImages, ReferencesRemainStableAndBelongToTheirFrame)
{
    OD::Gfx::ImGuiImages images;
    OD::Gfx::ImGuiImages otherFrame;
    void* first = images.ColorAttachment(42, 1);
    for(uint32_t i = 0; i < 4096; ++i) images.ColorAttachment(i, 0);

    uint32_t framebuffer = 0, attachment = 0;
    ASSERT_TRUE(images.Find(first, framebuffer, attachment));
    EXPECT_EQ(framebuffer, 42u);
    EXPECT_EQ(attachment, 1u);
    EXPECT_FALSE(otherFrame.Find(first, framebuffer, attachment));
    images.Clear();
    EXPECT_FALSE(images.Find(first, framebuffer, attachment));
}

TEST(ImGuiImages, ResolvesOnlyRegisteredImagesAndSkipsUnavailableAttachments)
{
    OD::Gfx::ImGuiImages images;
    ImDrawList list(nullptr);
    ImDrawCmd command;
    command.ElemCount = 6;
    command.ClipRect = ImVec4(0, 0, 100, 100);
    command.TextureId = images.ColorAttachment(42, 1);
    list.CmdBuffer.push_back(command);
    command.TextureId = reinterpret_cast<ImTextureID>(uintptr_t(123)); // Font/native image.
    list.CmdBuffer.push_back(command);
    command.TextureId = images.ColorAttachment(43, 0); // Unavailable image.
    list.CmdBuffer.push_back(command);
    command.TextureId = images.ColorAttachment(44, 0);
    command.UserCallback = ImDrawCallback_ResetRenderState;
    list.CmdBuffer.push_back(command);

    ImDrawData data;
    data.CmdLists.push_back(&list);
    data.CmdListsCount = 1;
    unsigned calls = 0;
    OD::Gfx::ResolveImGuiImages(data, images, [&](uint32_t fb, uint32_t attachment) -> void* {
        ++calls;
        if(fb != 42) return nullptr;
        EXPECT_EQ(attachment, 1u);
        return reinterpret_cast<void*>(uintptr_t(456));
    });

    EXPECT_EQ(calls, 2u);
    EXPECT_EQ(list.CmdBuffer[0].TextureId, reinterpret_cast<void*>(uintptr_t(456)));
    EXPECT_EQ(list.CmdBuffer[0].ElemCount, 6u);
    EXPECT_EQ(list.CmdBuffer[1].TextureId, reinterpret_cast<void*>(uintptr_t(123)));
    EXPECT_EQ(list.CmdBuffer[1].ElemCount, 6u);
    EXPECT_EQ(list.CmdBuffer[2].ElemCount, 0u);
    EXPECT_EQ(list.CmdBuffer[2].ClipRect.z, list.CmdBuffer[2].ClipRect.x);
    EXPECT_EQ(list.CmdBuffer[3].UserCallback, ImDrawCallback_ResetRenderState);
}

TEST(ImGuiImages, OffscreenRenderingFinishesBeforeTheImGuiWindowPass)
{
    OD::Gfx::RenderFrame frame;
    frame.renderCommands.BeginFramebuffer(42, 0, 0, true, OD::Gfx::ClearFlags::Color, {});
    frame.renderCommands.EndFramebuffer();
    frame.RecordImGuiDrawData(nullptr, nullptr);

    using Type = OD::Gfx::CommandBuffer::Type;
    const auto& commands = frame.renderCommands.commands;
    ASSERT_EQ(commands.size(), 5u);
    EXPECT_EQ(commands[0].type, Type::BeginFramebuffer);
    EXPECT_EQ(commands[1].type, Type::EndFramebuffer);
    EXPECT_EQ(commands[2].type, Type::BeginWindowFramebuffer);
    EXPECT_EQ(commands[3].type, Type::RenderImGui);
    EXPECT_EQ(commands[4].type, Type::EndFramebuffer);
    frame.Clear();
}

TEST(ImGuiImages, ReusesTheFinalWindowPassAndClearsFrameReferences)
{
    OD::Gfx::RenderFrame frame;
    frame.renderCommands.BeginWindowFramebuffer();
    frame.renderCommands.EndFramebuffer();
    void* image = frame.imguiImages.ColorAttachment(42, 0);
    frame.RecordImGuiDrawData(nullptr, nullptr);
    frame.RecordImGuiDrawData(nullptr, nullptr);

    using Type = OD::Gfx::CommandBuffer::Type;
    const auto& commands = frame.renderCommands.commands;
    ASSERT_EQ(commands.size(), 4u);
    EXPECT_EQ(commands[0].type, Type::BeginWindowFramebuffer);
    EXPECT_EQ(commands[1].type, Type::RenderImGui);
    EXPECT_EQ(commands[1].renderImGui.snapshotIndex, 0u);
    EXPECT_EQ(commands[2].type, Type::RenderImGui);
    EXPECT_EQ(commands[2].renderImGui.snapshotIndex, 1u);
    EXPECT_EQ(commands[3].type, Type::EndFramebuffer);
    frame.Clear();
    uint32_t framebuffer, attachment;
    EXPECT_FALSE(frame.imguiImages.Find(image, framebuffer, attachment));
    EXPECT_TRUE(frame.imguiSnapshots.empty());
    EXPECT_TRUE(frame.renderCommands.commands.empty());
}
