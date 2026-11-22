#include <gtest/gtest.h>
#include "OD/Gfx/Gfx.h"

using namespace OD::Gfx;

TEST(GfxCommandDispatch, DeviceMakesStartupModeAvailableToBackend){
    class TestDevice : public Device{
    public:
        RenderingMode modeSeenDuringInit = RenderingMode::Threaded;
    private:
        void InitializeRendering() override { modeSeenDuringInit = GetRenderingMode(); }
    } device;

    device.Init(RenderingMode::Immediate);

    EXPECT_EQ(device.modeSeenDuringInit, RenderingMode::Immediate);
    EXPECT_EQ(device.GetRenderingMode(), RenderingMode::Immediate);
}

#ifdef TEST_NEW_CMD
TEST(GfxCommandDispatch, PackedDrawAndUploadReplayTheirPayloads){
    CommandBuffer draw;
    ResourceCommands resources;
    int value = 123;
    draw.DrawIndexed(17);
    resources.UpdatedBuffer(5, &value, sizeof(value));
    value = 456;

    EXPECT_EQ(draw.commands.GetUsedBytes(), 8u);
    draw.commands.ForEach([](CommandBuffer::Type type, const void* payload){
        ASSERT_EQ(type, CommandBuffer::Type::DrawIndexed);
        auto indices = CommandBuffer::FastCommands::Read<decltype(CommandBuffer::Command::drawIndexed)>(payload);
        EXPECT_EQ(indices.indexCount, 17u);
    });
    resources.commands.ForEach([](ResourceCommands::Type type, const void* payload){
        ASSERT_EQ(type, ResourceCommands::Type::UpdateBuffer);
        auto upload = ResourceCommands::FastCommands::Read<decltype(ResourceCommands::Command::updateBuffer)>(payload);
        EXPECT_EQ(upload.id, 5u);
        int recorded = 0;
        std::memcpy(&recorded, upload.data, sizeof(recorded));
        EXPECT_EQ(recorded, 123);
    });
}
#else
TEST(GfxCommandDispatch, DrawCallsHandlerBeforeReturningInImmediateMode){
    CommandBuffer commands;
    uint32_t indexCount = 0;
    commands.SetImmediateExecutor(&indexCount, [](void* context, const CommandBuffer::Command& cmd){
        ASSERT_EQ(cmd.type, CommandBuffer::Type::DrawIndexed);
        *static_cast<uint32_t*>(context) = cmd.drawIndexed.indexCount;
    });

    commands.DrawIndexed(17);

    EXPECT_EQ(indexCount, 17u);
    EXPECT_TRUE(commands.commands.empty());
}

TEST(GfxCommandDispatch, ThreadedDrawsKeepRecordingOrder){
    CommandBuffer commands;
    commands.DrawIndexed(17);
    commands.DrawIndexed(42);

    ASSERT_EQ(commands.commands.size(), 2u);
    EXPECT_EQ(commands.commands[0].drawIndexed.indexCount, 17u);
    EXPECT_EQ(commands.commands[1].drawIndexed.indexCount, 42u);
}

TEST(GfxCommandDispatch, ImmediateUploadUsesCallerDataAndThreadedUploadOwnsCopy){
    ResourceCommands immediate;
    ResourceCommands threaded;
    const void* upload = nullptr;
    immediate.SetImmediateExecutor(&upload, [](void* context, const ResourceCommands::Command& cmd){
        ASSERT_EQ(cmd.type, ResourceCommands::Type::UpdateBuffer);
        *static_cast<const void**>(context) = cmd.updateBuffer.data;
    });
    int value = 123;

    immediate.UpdatedBuffer(5, &value, sizeof(value));
    threaded.UpdatedBuffer(5, &value, sizeof(value));
    EXPECT_EQ(upload, &value);
    EXPECT_TRUE(immediate.commands.empty());

    value = 456;
    ASSERT_EQ(threaded.commands.size(), 1u);
    int recorded = 0;
    std::memcpy(&recorded, threaded.commands.front().updateBuffer.data, sizeof(recorded));
    EXPECT_EQ(recorded, 123);
}

TEST(GfxCommandDispatch, ImmediateImGuiUsesAColorPreservingOverlay){
    struct OverlayCommands{
        std::vector<CommandBuffer::Type> types;
        bool preserveColor = false;
    } overlay;
    RenderFrame frame;
    frame.renderCommands.SetImmediateExecutor(&overlay, [](void* context, const CommandBuffer::Command& cmd){
        auto& output = *static_cast<OverlayCommands*>(context);
        output.types.push_back(cmd.type);
        if(cmd.type == CommandBuffer::Type::BeginWindowFramebuffer){
            output.preserveColor = cmd.beginWindowFramebuffer.preserveColor;
        }
    });

    frame.RecordImGuiDrawData(nullptr, nullptr);
    EXPECT_TRUE(overlay.types.empty());
    frame.DrawImGuiOverlay();

    EXPECT_EQ(overlay.types, (std::vector<CommandBuffer::Type>{
        CommandBuffer::Type::BeginWindowFramebuffer,
        CommandBuffer::Type::RenderImGui,
        CommandBuffer::Type::EndFramebuffer
    }));
    EXPECT_TRUE(overlay.preserveColor);
}
#endif

TEST(GfxCommandDispatch, PublicUploadCallsCountPayloadBytes){
    class TestDevice : public Device{
    public:
        int bufferWrites = 0;
    private:
        void UpdateBufferImpl(Buffer, const void*, size_t) override { ++bufferWrites; }
    } device;

    std::uint64_t uploadSize = 0;
    device.SetUploadCounter(&uploadSize);

    const int data[6]{};
    device.CreateBuffer(1024, BufferUsage::Vertex, BufferMemory::GPUOnly);
    device.UpdatedBuffer(1, data, sizeof(int));
    device.UploadTexture2D(2, data, sizeof(int));
    device.UploadTexture2DArray(3, data, sizeof(int), 0);
    device.UploadCubemap(4, data, sizeof(data));
    device.UploadCubemap(4, data, sizeof(int), static_cast<CubemapFace>(0));
    device.UpdatedBuffer(1, data, 0);
    device.UpdatedBuffer(1, nullptr, sizeof(data));

    EXPECT_EQ(uploadSize, 10 * sizeof(int));
    EXPECT_EQ(device.bufferWrites, 3);

    uploadSize = 0;
    device.UpdatedBuffer(1, data, sizeof(data));
    EXPECT_EQ(uploadSize, sizeof(data));
}
