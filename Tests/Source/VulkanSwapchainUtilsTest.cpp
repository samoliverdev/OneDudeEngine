#include <gtest/gtest.h>
#include <vulkan/vulkan.h>

#include <OD/Platform/VulkangGPU/VulkanSwapchainUtils.h>

TEST(VulkanSwapchainUtils, RecreatesForResizeOrSwapchainStatus)
{
    const VkExtent2D current{800, 600};
    const VkExtent2D resized{1280, 720};

    EXPECT_TRUE(OD::Gfx::NeedsSwapchainRecreation(VK_SUCCESS, current, resized));
    EXPECT_TRUE(OD::Gfx::NeedsSwapchainRecreation(VK_ERROR_OUT_OF_DATE_KHR, current, current));
    EXPECT_TRUE(OD::Gfx::NeedsSwapchainRecreation(VK_SUBOPTIMAL_KHR, current, current));
    EXPECT_FALSE(OD::Gfx::NeedsSwapchainRecreation(VK_SUCCESS, current, current));
}

TEST(VulkanSwapchainUtils, DoesNotRecreateWhileWindowIsMinimized)
{
    const VkExtent2D current{800, 600};
    const VkExtent2D minimized{0, 0};

    EXPECT_FALSE(OD::Gfx::NeedsSwapchainRecreation(VK_SUCCESS, current, minimized));
}
