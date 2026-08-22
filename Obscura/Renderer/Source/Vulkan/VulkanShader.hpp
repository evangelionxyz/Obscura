#pragma once

#include <Obscura/API.hpp>
#include <Umbra/ShaderCompiler.h>

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
    #ifndef VK_USE_PLATFORM_WIN32_KHR
        #define VK_USE_PLATFORM_WIN32_KHR
    #endif
#endif
#include <vulkan/vulkan.h>

#include <filesystem>
#include <vector>

namespace Obscura
{
    // Wraps a single compiled GLSL shader stage: SPIRV bytecode, VkShaderModule,
    // and full SPIRV reflection metadata (vertex attributes, descriptor bindings, push constants).
    class OBSCURA_RENDERER_API VulkanShader
    {
    public:
        VulkanShader() = default;
        ~VulkanShader() = default;

        // Compile GLSL from 'filepath', create VkShaderModule, and reflect SPIRV.
        // Returns true on success. Logs errors via Obscura logger on failure.
        bool Create(VkDevice device, const std::filesystem::path& filepath, UMBRA_ShaderType shaderType);

        void Destroy();

        [[nodiscard]] bool               IsValid()          const noexcept { return m_Module != VK_NULL_HANDLE; }
        [[nodiscard]] VkShaderModule     GetModule()        const noexcept { return m_Module; }
        [[nodiscard]] UMBRA_ShaderType   GetShaderType()    const noexcept { return m_ShaderType; }
        [[nodiscard]] const umbra::ShaderReflectionInfo& GetReflection() const noexcept { return m_Reflection; }
        [[nodiscard]] const std::vector<uint8_t>&        GetSpirv()      const noexcept { return m_Spirv; }

    private:
        VkDevice                         m_Device     = VK_NULL_HANDLE;
        VkShaderModule                   m_Module     = VK_NULL_HANDLE;
        UMBRA_ShaderType                 m_ShaderType = UMBRA_SHADER_TYPE_VERTEX;
        std::vector<uint8_t>             m_Spirv;
        umbra::ShaderReflectionInfo      m_Reflection;
    };
}
