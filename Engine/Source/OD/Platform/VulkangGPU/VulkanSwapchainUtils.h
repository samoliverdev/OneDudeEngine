#pragma once

#include <vulkan/vulkan.h>

namespace OD::Gfx {

inline bool NeedsSwapchainRecreation(VkResult swapchainResult, VkExtent2D currentExtent, VkExtent2D requestedExtent)
{
    if(requestedExtent.width == 0 || requestedExtent.height == 0)
        return false;

    return swapchainResult == VK_ERROR_OUT_OF_DATE_KHR ||
        swapchainResult == VK_SUBOPTIMAL_KHR ||
        currentExtent.width != requestedExtent.width ||
        currentExtent.height != requestedExtent.height;
}

}
