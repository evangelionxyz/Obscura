#include "VulkanRHI.hpp"

#include <Obscura/API.hpp>
#include <Obscura/IRHI.hpp>
#include <Obscura/Logger.hpp>
#include <Obscura/Types.hpp>

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace Obscura
{
    static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        (void)pUserData;

        std::string typeStr;
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        {
            typeStr += " General";
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        {
            typeStr += " Validation";
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        {
            typeStr += " Performance";
        }

        const char* idName = (pCallbackData->pMessageIdName && pCallbackData->pMessageIdName[0] != '\0') ? pCallbackData->pMessageIdName : "None";
        const char* msg = pCallbackData->pMessage ? pCallbackData->pMessage : "";

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            LOG_ERROR("[VulkanValidation][Type:{}][ID: {} (0x{:x})]: {}",
                typeStr.empty() ? " General" : typeStr, idName, static_cast<uint32_t>(pCallbackData->messageIdNumber), msg);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            LOG_WARN("[VulkanValidation][Type:{}][ID: {} (0x{:x})]: {}",
                typeStr.empty() ? " General" : typeStr, idName, static_cast<uint32_t>(pCallbackData->messageIdNumber), msg);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            LOG_INFO("[VulkanValidation][Type:{}][ID: {} (0x{:x})]: {}",
                typeStr.empty() ? " General" : typeStr, idName, static_cast<uint32_t>(pCallbackData->messageIdNumber), msg);
        }
        else
        {
            LOG_TRACE("[VulkanValidation][Type:{}][ID: {} (0x{:x})]: {}",
                typeStr.empty() ? " General" : typeStr, idName, static_cast<uint32_t>(pCallbackData->messageIdNumber), msg);
        }

        if (pCallbackData->objectCount > 0)
        {
            for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
            {
                const auto& obj = pCallbackData->pObjects[i];
                LOG_DEBUG("  -> Object[{}]: Handle = 0x{:x}, Name = {}", i, obj.objectHandle, obj.pObjectName ? obj.pObjectName : "unnamed");
            }
        }

        return VK_FALSE;
    }

    bool VulkanRHI::CheckValidationLayerSupport(const std::vector<const char*>& layers)
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : layers)
        {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers)
            {
                if (std::strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound)
            {
                return false;
            }
        }
        return true;
    }

    void VulkanRHI::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = VulkanDebugCallback;
        createInfo.pUserData = nullptr;
    }

    void VulkanRHI::SetupDebugMessenger()
    {
        if (!m_EnableValidationLayers || m_Instance == VK_NULL_HANDLE)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        PopulateDebugMessengerCreateInfo(createInfo);

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            VkResult res = func(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
            if (res == VK_SUCCESS)
            {
                LOG_INFO("  [VulkanRHI] Vulkan Debug Utils Messenger initialized successfully.");
            }
            else
            {
                LOG_ERROR("  [VulkanRHI] Failed to set up VkDebugUtilsMessengerEXT with error code: {}", static_cast<int>(res));
            }
        }
        else
        {
            LOG_ERROR("  [VulkanRHI] vkCreateDebugUtilsMessengerEXT function pointer not found!");
        }
    }

    bool VulkanRHI::CreateInstance()
    {
        if (m_EnableValidationLayers && !CheckValidationLayerSupport(m_ValidationLayers))
        {
            LOG_WARN("  [VulkanRHI] Requested validation layers not available! Disabling validation.");
            m_EnableValidationLayers = false;
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Obscura App";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Obscura Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        std::vector<const char*> extensions =
        {
            VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
        };

        if (m_EnableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (m_EnableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
            createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        VkResult res = vkCreateInstance(&createInfo, nullptr, &m_Instance);
        if (res != VK_SUCCESS)
        {
            appInfo.apiVersion = VK_API_VERSION_1_0;
            res = vkCreateInstance(&createInfo, nullptr, &m_Instance);
            if (res != VK_SUCCESS)
            {
                LOG_ERROR("  [VulkanRHI] vkCreateInstance failed with error code: {}", static_cast<int>(res));
                return false;
            }
        }

        if (m_EnableValidationLayers)
        {
            SetupDebugMessenger();
        }

        LOG_INFO("  [VulkanRHI] VkInstance created successfully (Validation layers: {}).",
            m_EnableValidationLayers ? "ENABLED" : "DISABLED");
        return true;
    }

    bool VulkanRHI::SelectPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            LOG_ERROR("  [VulkanRHI] No Vulkan-compatible physical devices found.");
            return false;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        for (const auto &device : devices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            for (uint32_t i = 0; i < queueFamilyCount; ++i)
            {
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    m_PhysicalDevice = device;
                    m_GraphicsQueueFamily = i;
                    m_AvailableQueueCount = queueFamilies[i].queueCount;
                    LOG_INFO("  [VulkanRHI] Selected GPU: {} (Vulkan {}.{}.{}, Queue count: {})",
                        props.deviceName,
                        VK_API_VERSION_MAJOR(props.apiVersion),
                        VK_API_VERSION_MINOR(props.apiVersion),
                        VK_API_VERSION_PATCH(props.apiVersion),
                        m_AvailableQueueCount);
                    return true;
                }
            }
        }

        LOG_ERROR("  [VulkanRHI] No physical device with graphics queue found.");
        return false;
    }

    bool VulkanRHI::CreateLogicalDevice()
    {
        uint32_t requestedQueues = (std::min)(m_AvailableQueueCount, 2u);
        std::vector<float> queuePriorities(requestedQueues, 1.0f);

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = m_GraphicsQueueFamily;
        queueCreateInfo.queueCount = requestedQueues;
        queueCreateInfo.pQueuePriorities = queuePriorities.data();

        VkPhysicalDeviceFeatures deviceFeatures{};

        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (m_EnableValidationLayers)
        {
            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
            deviceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
        }
        else
        {
            deviceCreateInfo.enabledLayerCount = 0;
        }

        VkResult res = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device);
        if (res != VK_SUCCESS)
        {
            LOG_ERROR("  [VulkanRHI] vkCreateDevice failed with error code: {}", static_cast<int>(res));
            return false;
        }

        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
        m_QtQueueIndex = (requestedQueues >= 2) ? 1u : 0u;

        LOG_INFO("  [VulkanRHI] Logical VkDevice and graphics VkQueue created successfully (Queues allocated: {}, QtQueueIndex: {}).",
            requestedQueues, m_QtQueueIndex);
        return true;
    }

    bool VulkanRHI::CreateCommandObjects()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_GraphicsQueueFamily;

        if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
        {
            LOG_ERROR("  [VulkanRHI] Failed to create VkCommandPool.");
            return false;
        }

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = BACKBUFFER_COUNT;

        if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
        {
            LOG_ERROR("  [VulkanRHI] Failed to allocate VkCommandBuffers.");
            return false;
        }

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (std::uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
        {
            if (vkCreateFence(m_Device, &fenceInfo, nullptr, &m_RenderFences[i]) != VK_SUCCESS)
            {
                LOG_ERROR("  [VulkanRHI] Failed to create VkFence for buffer {}.", i);
                return false;
            }
        }

        return true;
    }

    uint32_t VulkanRHI::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if (typeFilter & (1 << i))
            {
                return i;
            }
        }

        return 0;
    }

    bool VulkanRHI::Initialize()
    {
        if (m_Initialized)
        {
            return true;
        }

        LOG_INFO("  [VulkanRHI] Initializing Vulkan RHI Backend...");

        if (!CreateInstance())
        {
            return false;
        }

        if (!SelectPhysicalDevice())
        {
            Shutdown();
            return false;
        }

        if (!CreateLogicalDevice())
        {
            Shutdown();
            return false;
        }

        if (!CreateCommandObjects())
        {
            Shutdown();
            return false;
        }

        m_Initialized = true;
        LOG_INFO("  [VulkanRHI] Vulkan RHI initialized successfully.");
        return true;
    }

    void VulkanRHI::Shutdown()
    {
        if (!m_Initialized && m_Instance == VK_NULL_HANDLE)
        {
            return;
        }

        LOG_INFO("  [VulkanRHI] Shutting down Vulkan RHI Backend...");

        DestroyOffscreenTarget();

        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);

            for (std::uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
            {
                if (m_RenderFences[i] != VK_NULL_HANDLE)
                {
                    vkDestroyFence(m_Device, m_RenderFences[i], nullptr);
                    m_RenderFences[i] = VK_NULL_HANDLE;
                }
            }

            if (m_CommandPool != VK_NULL_HANDLE)
            {
                vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
                m_CommandPool = VK_NULL_HANDLE;
                m_CommandBuffers.fill(VK_NULL_HANDLE);
            }

            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
            m_GraphicsQueue = VK_NULL_HANDLE;
            LOG_INFO("  [VulkanRHI] VkDevice destroyed.");
        }

        if (m_DebugMessenger != VK_NULL_HANDLE)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                func(m_Instance, m_DebugMessenger, nullptr);
            }
            m_DebugMessenger = VK_NULL_HANDLE;
            LOG_INFO("  [VulkanRHI] VkDebugUtilsMessengerEXT destroyed.");
        }

        if (m_Instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
            m_PhysicalDevice = VK_NULL_HANDLE;
            LOG_INFO("  [VulkanRHI] VkInstance destroyed.");
        }

        m_Initialized = false;
        LOG_INFO("  [VulkanRHI] Vulkan RHI shutdown complete.");
    }

    void VulkanRHI::BeginFrame()
    {
        if (m_RenderMode == RenderMode::Window)
        {
            LOG_TRACE("  [VulkanRHI] Begin Frame (Acquire Next Swapchain Image)");
        }
        else if (m_RenderMode == RenderMode::Offscreen)
        {
            RenderOffscreenFrame();
        }
    }

    void VulkanRHI::EndFrame()
    {
        if (m_RenderMode == RenderMode::Window)
        {
            LOG_TRACE("  [VulkanRHI] End Frame (Queue Present)");
        }
    }

    void VulkanRHI::RenderOffscreenFrame()
    {
        if (m_OffscreenImages[0] == VK_NULL_HANDLE || m_Device == VK_NULL_HANDLE)
        {
            return;
        }

        m_FrameCounter++;

        const std::uint32_t writeIdx = m_WriteIndex;

        VkResult res = vkWaitForFences(m_Device, 1, &m_RenderFences[writeIdx], VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device, 1, &m_RenderFences[writeIdx]);
        vkResetCommandBuffer(m_CommandBuffers[writeIdx], 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(m_CommandBuffers[writeIdx], &beginInfo);

        // Transition image layout to TRANSFER_DST_OPTIMAL for clearing
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = m_CurrentLayouts[writeIdx];
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_OffscreenImages[writeIdx];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = (m_CurrentLayouts[writeIdx] == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ? VK_ACCESS_SHADER_READ_BIT : 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(m_CommandBuffers[writeIdx],
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        m_CurrentLayouts[writeIdx] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        VkClearColorValue clearColor = { .float32 = { 0.4f, 0.4f, 0.4f, 1.0f } };
        VkImageSubresourceRange clearRange{};
        clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clearRange.baseMipLevel = 0;
        clearRange.levelCount = 1;
        clearRange.baseArrayLayer = 0;
        clearRange.layerCount = 1;

        vkCmdClearColorImage(m_CommandBuffers[writeIdx], m_OffscreenImages[writeIdx],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &clearRange);

        // Transition back to SHADER_READ_ONLY_OPTIMAL for Qt SceneGraph consumption
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(m_CommandBuffers[writeIdx],
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        m_CurrentLayouts[writeIdx] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkEndCommandBuffer(m_CommandBuffers[writeIdx]);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffers[writeIdx];

        vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_RenderFences[writeIdx]);

        // Double buffering swap
        m_ReadIndex = writeIdx;
        m_WriteIndex = (writeIdx + 1) % BACKBUFFER_COUNT;
    }

    bool VulkanRHI::CreateOffscreenTarget(std::uint32_t width, std::uint32_t height)
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            for (std::uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
            {
                if (m_RenderFences[i] != VK_NULL_HANDLE)
                {
                    vkWaitForFences(m_Device, 1, &m_RenderFences[i], VK_TRUE, UINT64_MAX);
                }
            }
            vkDeviceWaitIdle(m_Device);
        }

        DestroyOffscreenTarget();

        m_OffscreenWidth = (width > 0) ? width : 1280;
        m_OffscreenHeight = (height > 0) ? height : 720;

        if (m_Device == VK_NULL_HANDLE)
        {
            m_FrameBuffer.resize(static_cast<std::size_t>(m_OffscreenWidth) * m_OffscreenHeight * 4, 0);
            return true;
        }

        for (std::uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
        {
            // 1. Create GPU VkImage
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = m_OffscreenWidth;
            imageInfo.extent.height = m_OffscreenHeight;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = m_OffscreenFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

            VkResult res = vkCreateImage(m_Device, &imageInfo, nullptr, &m_OffscreenImages[i]);
            if (res != VK_SUCCESS)
            {
                LOG_ERROR("  [VulkanRHI] Failed to create offscreen VkImage [{}]: error {}", i, static_cast<int>(res));
                return false;
            }

            // 2. Allocate and bind device local memory
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(m_Device, m_OffscreenImages[i], &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            res = vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_OffscreenMemories[i]);
            if (res != VK_SUCCESS)
            {
                LOG_ERROR("  [VulkanRHI] Failed to allocate offscreen VkDeviceMemory [{}]: error {}", i, static_cast<int>(res));
                return false;
            }

            vkBindImageMemory(m_Device, m_OffscreenImages[i], m_OffscreenMemories[i], 0);

            // 3. Create VkImageView
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_OffscreenImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_OffscreenFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            res = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_OffscreenImageViews[i]);
            if (res != VK_SUCCESS)
            {
                LOG_ERROR("  [VulkanRHI] Failed to create offscreen VkImageView [{}]: error {}", i, static_cast<int>(res));
                return false;
            }

            m_CurrentLayouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        m_WriteIndex = 0;
        m_ReadIndex = 0;
        m_FrameBuffer.resize(static_cast<std::size_t>(m_OffscreenWidth) * m_OffscreenHeight * 4, 0);

        // Pre-render both buffers so both start in SHADER_READ_ONLY_OPTIMAL layout
        for (int i = 0; i < BACKBUFFER_COUNT; ++i)
        {
            RenderOffscreenFrame();
        }
        vkDeviceWaitIdle(m_Device);
        return true;
    }

    void VulkanRHI::DestroyOffscreenTarget()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);

            for (std::uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
            {
                if (m_OffscreenImageViews[i] != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(m_Device, m_OffscreenImageViews[i], nullptr);
                    m_OffscreenImageViews[i] = VK_NULL_HANDLE;
                }

                if (m_OffscreenImages[i] != VK_NULL_HANDLE)
                {
                    vkDestroyImage(m_Device, m_OffscreenImages[i], nullptr);
                    m_OffscreenImages[i] = VK_NULL_HANDLE;
                }

                if (m_OffscreenMemories[i] != VK_NULL_HANDLE)
                {
                    vkFreeMemory(m_Device, m_OffscreenMemories[i], nullptr);
                    m_OffscreenMemories[i] = VK_NULL_HANDLE;
                }

                m_CurrentLayouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
            }
        }

        m_FrameBuffer.clear();
        m_OffscreenWidth = 0;
        m_OffscreenHeight = 0;
    }

    bool VulkanRHI::ResizeOffscreenTarget(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return false;
        }
        if (width == m_OffscreenWidth && height == m_OffscreenHeight && m_OffscreenImages[0] != VK_NULL_HANDLE)
        {
            return true;
        }

        return CreateOffscreenTarget(width, height);
    }

    const void* VulkanRHI::GetFrameBufferData() const
    {
        return m_FrameBuffer.empty() ? nullptr : m_FrameBuffer.data();
    }

    std::uint32_t VulkanRHI::GetFrameBufferStride() const
    {
        return m_OffscreenWidth * 4;
    }

    std::uint32_t VulkanRHI::GetFrameBufferWidth() const
    {
        return m_OffscreenWidth;
    }

    std::uint32_t VulkanRHI::GetFrameBufferHeight() const
    {
        return m_OffscreenHeight;
    }

    GPUTextureHandle VulkanRHI::GetGPUTextureHandle() const
    {
        const std::uint32_t readIdx = m_ReadIndex;

        if (m_OffscreenImages[readIdx] == VK_NULL_HANDLE)
        {
            return {};
        }

        return GPUTextureHandle{
            .nativeHandle   = reinterpret_cast<void*>(m_OffscreenImages[readIdx]),
            .layoutOrState  = static_cast<int>(m_CurrentLayouts[readIdx]),
            .width          = m_OffscreenWidth,
            .height         = m_OffscreenHeight,
            .format         = static_cast<int>(m_OffscreenFormat),
            .isGPUInterop   = true
        };
    }

    VulkanDeviceObjects VulkanRHI::GetVulkanDeviceObjects() const
    {
        return VulkanDeviceObjects{
            .instance         = reinterpret_cast<void*>(m_Instance),
            .physicalDevice   = reinterpret_cast<void*>(m_PhysicalDevice),
            .device           = reinterpret_cast<void*>(m_Device),
            .graphicsQueue    = reinterpret_cast<void*>(m_GraphicsQueue),
            .queueFamilyIndex = m_GraphicsQueueFamily,
            .queueIndex       = m_QtQueueIndex
        };
    }

    void VulkanRHI::SetRenderMode(RenderMode mode)
    {
        m_RenderMode = mode;
        LOG_INFO("  [VulkanRHI] Render Mode set to: {}", mode == RenderMode::Offscreen ? "Offscreen" : "Window");
    }

    const char *VulkanRHI::GetName() const
    {
        return "Vulkan 1.3 (RHI Subsystem)";
    }

    void VulkanRHI::Destroy()
    {
        delete this;
    }
}

extern "C"
{
    OBSCURA_RENDERER_API Obscura::IRHI* CreateRHI(std::uint32_t abiVersion)
    {
        if (abiVersion != Obscura::RHI_ABI_VERSION)
        {
            LOG_ERROR("  [Renderer.dll] ABI Version mismatch! Requested version {}, DLL compiled with {}",
                abiVersion, Obscura::RHI_ABI_VERSION);
            return nullptr;
        }
        return new Obscura::VulkanRHI();
    }

    OBSCURA_RENDERER_API void DestroyRHI(Obscura::IRHI* rhi)
    {
        if (rhi)
        {
            rhi->Destroy();
        }
    }
}
