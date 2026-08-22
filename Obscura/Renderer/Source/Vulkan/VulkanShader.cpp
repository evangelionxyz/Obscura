#include "VulkanShader.hpp"

#include <Obscura/Logger.hpp>

#include <fstream>
#include <sstream>

namespace Obscura
{
    bool VulkanShader::Create(VkDevice device, const std::filesystem::path& filepath, UMBRA_ShaderType shaderType)
    {
        m_Device     = device;
        m_ShaderType = shaderType;

        if (!std::filesystem::exists(filepath))
        {
            LOG_ERROR("[VulkanShader] Shader file not found: {}", filepath.string());
            return false;
        }

        // Build CompilerOptions for GLSL -> SPIRV compilation
        umbra::CompilerOptions options = {};
        options.compilerType                  = UMBRA_SHADER_COMPILER_TYPE_DXC;
        options.platformType                  = UMBRA_SHADER_PLATFORM_TYPE_SPIRV;
        options.filepath                      = filepath;
        options.outputFilepath                = filepath.parent_path();
        options.shaderDesc.entryPoint         = "main";
        options.shaderDesc.shaderModel        = "6_5";
        options.shaderDesc.vulkanVersion      = "1.3";
        options.shaderDesc.shaderType         = shaderType;
        options.shaderDesc.optLevel           = UMBRA_OPT_LEVEL_0; // No optimization for dev build
        options.tRegShift                     = 0;
        options.sRegShift                     = 0;
        options.bRegShift                     = 0;
        options.uRegShift                     = 0;

        // Compile GLSL to SPIRV
        try
        {
            m_Spirv = umbra::ShaderCompiler::CompileGLSL(options);
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR("[VulkanShader] Compilation exception for '{}': {}", filepath.string(), ex.what());
            return false;
        }
        catch (...)
        {
            LOG_ERROR("[VulkanShader] Unknown compilation exception for '{}'", filepath.string());
            return false;
        }

        if (m_Spirv.empty())
        {
            LOG_ERROR("[VulkanShader] Compilation produced no output for '{}'", filepath.string());
            return false;
        }

        LOG_INFO("[VulkanShader] Compiled '{}' -> {} bytes SPIRV", filepath.filename().string(), m_Spirv.size());

        // Reflect SPIRV
        try
        {
            m_Reflection = umbra::ShaderReflection::SPIRVReflect(shaderType, m_Spirv);
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR("[VulkanShader] SPIRV reflection failed for '{}': {}", filepath.string(), ex.what());
            return false;
        }

        LOG_INFO("[VulkanShader] Reflection: UBOs={}, Samplers={}, PushConstants={}, Inputs={}, Outputs={}",
            m_Reflection.numUniformBuffers,
            m_Reflection.numSamplers,
            m_Reflection.numPushConstants,
            m_Reflection.numStageInputs,
            m_Reflection.numStageOutputs);

        // Create VkShaderModule
        VkShaderModuleCreateInfo moduleInfo{};
        moduleInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleInfo.codeSize = m_Spirv.size();
        moduleInfo.pCode    = reinterpret_cast<const uint32_t*>(m_Spirv.data());

        VkResult res = vkCreateShaderModule(m_Device, &moduleInfo, nullptr, &m_Module);
        if (res != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanShader] vkCreateShaderModule failed ({}): '{}'", static_cast<int>(res), filepath.string());
            return false;
        }

        LOG_INFO("[VulkanShader] VkShaderModule created for '{}'", filepath.filename().string());
        return true;
    }

    void VulkanShader::Destroy()
    {
        if (m_Module != VK_NULL_HANDLE && m_Device != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_Device, m_Module, nullptr);
            m_Module = VK_NULL_HANDLE;
        }
        m_Spirv.clear();
        m_Reflection = {};
    }
}
