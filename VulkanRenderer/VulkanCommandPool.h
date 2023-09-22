#pragma once
#include "main.cpp"
#include "VulkanDevice.h"
#include "Window.h"

class VulkanCommandPool
{
public:
    VkCommandPool commandPool;
    VulkanCommandPool(VulkanDevice vulkanDevice, Window window)
    {
        createCommandPool(vulkanDevice, window);
    }
private:
    void createCommandPool(VulkanDevice vulkanDevice, Window window)
    {
        QueueFamilyIndices queueFamilyIndices = VulkanDevice::findQueueFamilies(vulkanDevice.physicalDevice, window.surface);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(vulkanDevice.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Unablw to create command pool.");
        }
    }
};

