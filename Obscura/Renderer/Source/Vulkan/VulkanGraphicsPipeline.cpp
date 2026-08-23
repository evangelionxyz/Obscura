#include "VulkanGraphicsPipeline.hpp"
#include "VulkanUtils.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <vector>

namespace Obscura
{
    // Map UMBRA_VertexElementFormat -> VkFormat
    static VkFormat UmbraFormatToVk(UMBRA_VertexElementFormat fmt)
    {
        switch (fmt)
        {
        case UMBRA_VERTEX_ELEMENT_FORMAT_FLOAT:  return VK_FORMAT_R32_SFLOAT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_FLOAT2: return VK_FORMAT_R32G32_SFLOAT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_FLOAT3: return VK_FORMAT_R32G32B32_SFLOAT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_FLOAT4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_INT:    return VK_FORMAT_R32_SINT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_INT2:   return VK_FORMAT_R32G32_SINT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_INT3:   return VK_FORMAT_R32G32B32_SINT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_INT4:   return VK_FORMAT_R32G32B32A32_SINT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_UINT:   return VK_FORMAT_R32_UINT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_UINT2:  return VK_FORMAT_R32G32_UINT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_UINT3:  return VK_FORMAT_R32G32B32_UINT;
        case UMBRA_VERTEX_ELEMENT_FORMAT_UINT4:  return VK_FORMAT_R32G32B32A32_UINT;
        default: return VK_FORMAT_R32G32B32_SFLOAT; // Safe fallback
        }
    }

    // Returns byte size of an UMBRA vertex element format.
    static uint32_t UmbraFormatSize(UMBRA_VertexElementFormat fmt)
    {
        switch (fmt)
        {
        case UMBRA_VERTEX_ELEMENT_FORMAT_FLOAT:
        case UMBRA_VERTEX_ELEMENT_FORMAT_INT:
        case UMBRA_VERTEX_ELEMENT_FORMAT_UINT:   return 4;
        case UMBRA_VERTEX_ELEMENT_FORMAT_FLOAT2:
        case UMBRA_VERTEX_ELEMENT_FORMAT_INT2:
        case UMBRA_VERTEX_ELEMENT_FORMAT_UINT2:  return 8;
        case UMBRA_VERTEX_ELEMENT_FORMAT_FLOAT3:
        case UMBRA_VERTEX_ELEMENT_FORMAT_INT3:
        case UMBRA_VERTEX_ELEMENT_FORMAT_UINT3:  return 12;
        case UMBRA_VERTEX_ELEMENT_FORMAT_FLOAT4:
        case UMBRA_VERTEX_ELEMENT_FORMAT_INT4:
        case UMBRA_VERTEX_ELEMENT_FORMAT_UINT4:  return 16;
        default: return 12;
        }
    }

    // ---------------------------------------------------------------------------
    // Create
    // ---------------------------------------------------------------------------
    bool VulkanGraphicsPipeline::Create(
        GraphicsPipelineCreateInfo                          &info,
        VkDevice                                            device,
        const VulkanShader&                                 vertShader,
        const VulkanShader&                                 fragShader,
        VkFormat                                            targetFormat,
        uint32_t                                            width,
        uint32_t                                            height,
        const std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT>& imageViews,
        const std::vector<VkDescriptorSetLayout>&           externalDescriptorSetLayouts)
    {
        m_Device = device;
        m_Width  = width;
        m_Height = height;

        if (targetFormat == VK_FORMAT_UNDEFINED)
        {
            targetFormat = VK_FORMAT_R8G8B8A8_UNORM;
        }

        if (!BuildRenderPass(targetFormat))
            return false;

        if (!BuildPipelineLayout(vertShader, fragShader, externalDescriptorSetLayouts))
            return false;

        if (!BuildPipeline(info, vertShader, fragShader))
            return false;

        if (width > 0 && height > 0 && imageViews[0] != VK_NULL_HANDLE)
        {
            if (!BuildFramebuffers(imageViews, width, height))
                return false;
        }

        LOG_INFO("[VulkanGraphicsPipeline] Graphics pipeline created ({}x{})", width, height);
        return true;
    }

    // ---------------------------------------------------------------------------
    // BuildRenderPass
    // ---------------------------------------------------------------------------
    bool VulkanGraphicsPipeline::BuildRenderPass(VkFormat format)
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format         = format;
        colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorRef;

        // Subpass dependency: ensure image is ready before color writes and transitioned after
        std::array<VkSubpassDependency, 2> dependencies{};

        // External -> Subpass 0: wait for any prior shader reads to finish
        dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass      = 0;
        dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Subpass 0 -> External: ensure color writes are visible before shader reads
        dependencies[1].srcSubpass      = 0;
        dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo rpInfo{};
        rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.attachmentCount = 1;
        rpInfo.pAttachments    = &colorAttachment;
        rpInfo.subpassCount    = 1;
        rpInfo.pSubpasses      = &subpass;
        rpInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        rpInfo.pDependencies   = dependencies.data();

        VkResult res = vkCreateRenderPass(m_Device, &rpInfo, nullptr, &m_RenderPass);
        if (res != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanGraphicsPipeline] vkCreateRenderPass failed: {}", static_cast<int>(res));
            return false;
        }
        return true;
    }

    // ---------------------------------------------------------------------------
    // BuildPipelineLayout — driven by SPIRV reflection
    // ---------------------------------------------------------------------------
    bool VulkanGraphicsPipeline::BuildPipelineLayout(const VulkanShader& vertShader,
                                                     const VulkanShader& fragShader,
                                                     const std::vector<VkDescriptorSetLayout>& externalLayouts)
    {
        if (!externalLayouts.empty())
        {
            m_DescriptorSetLayouts = externalLayouts;
            m_OwnsDescriptorSetLayouts = false;
        }
        else
        {
            m_OwnsDescriptorSetLayouts = true;
            const auto& vertRefl = vertShader.GetReflection();
            const auto& fragRefl = fragShader.GetReflection();

            // Collect descriptor bindings per set from both stages.
            // Key: set index. Value: list of bindings.
            std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setBindings;

            auto AddBindings = [&](const std::vector<umbra::ShaderResourceInfo>& resources,
                                   VkDescriptorType                               descType,
                                   VkShaderStageFlags                             stageFlags)
            {
                for (const auto& res : resources)
                {
                    VkDescriptorSetLayoutBinding binding{};
                    binding.binding            = res.binding;
                    binding.descriptorType     = descType;
                    binding.descriptorCount    = res.count;
                    binding.stageFlags         = stageFlags;
                    binding.pImmutableSamplers = nullptr;
                    setBindings[res.set].push_back(binding);
                }
            };

            // Vertex stage resources
            AddBindings(vertRefl.uniformBuffers,  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT);
            AddBindings(vertRefl.sampledImages,   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT);
            AddBindings(vertRefl.storageBuffers,  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT);

            // Fragment stage resources (merge stage flags if binding already present)
            AddBindings(fragRefl.uniformBuffers,  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT);
            AddBindings(fragRefl.sampledImages,   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
            AddBindings(fragRefl.storageBuffers,  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT);

            // Create one VkDescriptorSetLayout per set
            uint32_t maxSet = setBindings.empty() ? 0u : (setBindings.rbegin()->first + 1u);
            m_DescriptorSetLayouts.resize(maxSet, VK_NULL_HANDLE);

            for (uint32_t setIdx = 0; setIdx < maxSet; ++setIdx)
            {
                auto it = setBindings.find(setIdx);
                VkDescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                if (it != setBindings.end())
                {
                    layoutInfo.bindingCount = static_cast<uint32_t>(it->second.size());
                    layoutInfo.pBindings    = it->second.data();
                }

                VkResult res = vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayouts[setIdx]);
                if (res != VK_SUCCESS)
                {
                    LOG_ERROR("[VulkanGraphicsPipeline] Failed to create descriptor set layout for set {}", setIdx);
                    return false;
                }
            }
        }

        const auto& vertRefl = vertShader.GetReflection();
        const auto& fragRefl = fragShader.GetReflection();

        // Collect push constant ranges from both stages
        std::vector<VkPushConstantRange> pushRanges;

        auto AddPushConstants = [&](const std::vector<umbra::ShaderPushConstantInfo>& pcs,
                                    VkShaderStageFlags                                 stage)
        {
            uint32_t offset = 0;
            for (const auto& pc : pcs)
            {
                VkPushConstantRange range{};
                range.stageFlags = stage;
                range.offset     = offset;
                range.size       = pc.size;
                pushRanges.push_back(range);
                offset += pc.size;
            }
        };

        AddPushConstants(vertRefl.pushConstants, VK_SHADER_STAGE_VERTEX_BIT);
        AddPushConstants(fragRefl.pushConstants, VK_SHADER_STAGE_FRAGMENT_BIT);

        // If reflection returns no push constants but we know we have them (design D3: 128 bytes),
        // add a fallback covering both stages. This handles shaderc/SPIRV-cross not reporting
        // the block name when it is unnamed or using default layout.
        if (pushRanges.empty())
        {
            LOG_INFO("[VulkanGraphicsPipeline] No push constants reflected — adding 128-byte fallback for MVP matrices.");
            VkPushConstantRange fallback{};
            fallback.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            fallback.offset     = 0;
            fallback.size       = 128; // 2x mat4
            pushRanges.push_back(fallback);
        }

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount         = static_cast<uint32_t>(m_DescriptorSetLayouts.size());
        layoutInfo.pSetLayouts            = m_DescriptorSetLayouts.empty() ? nullptr : m_DescriptorSetLayouts.data();
        layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushRanges.size());
        layoutInfo.pPushConstantRanges    = pushRanges.empty() ? nullptr : pushRanges.data();

        VkResult res = vkCreatePipelineLayout(m_Device, &layoutInfo, nullptr, &m_PipelineLayout);
        if (res != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanGraphicsPipeline] vkCreatePipelineLayout failed: {}", static_cast<int>(res));
            return false;
        }

        LOG_INFO("[VulkanGraphicsPipeline] Pipeline layout: {} descriptor sets, {} push constant ranges",
            m_DescriptorSetLayouts.size(), pushRanges.size());
        return true;
    }

    // ---------------------------------------------------------------------------
    // BuildPipeline — vertex input from reflection, everything else from design
    // ---------------------------------------------------------------------------
    bool VulkanGraphicsPipeline::BuildPipeline(GraphicsPipelineCreateInfo &info,
                                               const VulkanShader& vertShader,
                                               const VulkanShader& fragShader)
    {
        // --- Shader stages ---
        std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
        stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertShader.GetModule();
        stages[0].pName  = "main";

        stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragShader.GetModule();
        stages[1].pName  = "main";

        // --- Vertex input from SPIRV reflection ---
        const auto& vertRefl = vertShader.GetReflection();

        std::vector<VkVertexInputAttributeDescription> attrDescs;
        uint32_t stride = 0;

        if (!vertRefl.vertexAttributes.empty())
        {
            std::vector<umbra::VertexAttribute> sortedAttrs(vertRefl.vertexAttributes);
            std::sort(sortedAttrs.begin(), sortedAttrs.end(),
                      [](const umbra::VertexAttribute& a, const umbra::VertexAttribute& b)
                      { return a.location < b.location; });

            attrDescs.reserve(sortedAttrs.size());
            uint32_t currentOffset = 0;
            for (const auto& attr : sortedAttrs)
            {
                VkVertexInputAttributeDescription desc{};
                desc.location = attr.location;
                desc.binding  = 0; // Single binding
                desc.format   = UmbraFormatToVk(attr.format);
                desc.offset   = currentOffset;
                attrDescs.push_back(desc);

                const uint32_t fmtSize = UmbraFormatSize(attr.format);
                currentOffset          += fmtSize;
                stride                 += fmtSize;
            }
        }
        else if (!vertRefl.stageInputs.empty())
        {
            // SPIR-V reflection populates stageInputs, not vertexAttributes.
            // Sort by location and derive packed byte offsets cumulatively.
            LOG_INFO("[VulkanGraphicsPipeline] vertexAttributes empty — deriving layout from stageInputs.");
            std::vector<umbra::ShaderStageIOInfo> sortedInputs(vertRefl.stageInputs);
            std::sort(sortedInputs.begin(), sortedInputs.end(),
                      [](const umbra::ShaderStageIOInfo& a, const umbra::ShaderStageIOInfo& b)
                      { return a.location < b.location; });

            attrDescs.reserve(sortedInputs.size());
            uint32_t currentOffset = 0;
            for (const auto& input : sortedInputs)
            {
                VkVertexInputAttributeDescription desc{};
                desc.location = input.location;
                desc.binding  = 0;
                desc.format   = UmbraFormatToVk(input.format);
                desc.offset   = currentOffset;
                attrDescs.push_back(desc);

                const uint32_t fmtSize = UmbraFormatSize(input.format);
                currentOffset          += fmtSize;
                stride                 += fmtSize;
            }
        }

        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding   = 0;
        bindingDesc.stride    = stride;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount   = 1;
        vertexInputInfo.pVertexBindingDescriptions      = &bindingDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
        vertexInputInfo.pVertexAttributeDescriptions    = attrDescs.data();

        // --- Input assembly ---
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology               = static_cast<VkPrimitiveTopology>(info.primitive);
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // --- Viewport & scissor (Dynamic State) ---
        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = (m_Width > 0) ? static_cast<float>(m_Width) : 1280.0f;
        viewport.height   = (m_Height > 0) ? static_cast<float>(m_Height) : 720.0f;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = {
            (m_Width > 0) ? m_Width : 1280u,
            (m_Height > 0) ? m_Height : 720u
        };

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports    = &viewport;
        viewportState.scissorCount  = 1;
        viewportState.pScissors     = &scissor;

        std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates    = dynamicStates.data();

        // --- Rasterizer ---
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable        = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode             = static_cast<VkPolygonMode>(info.fillMode);
        rasterizer.cullMode                = static_cast<VkCullModeFlagBits>(info.cullMode);
        rasterizer.frontFace               = static_cast<VkFrontFace>(info.frontFace);
        rasterizer.depthBiasEnable         = VK_FALSE;
        rasterizer.lineWidth               = info.lineWidth;

        // --- Multisampling ---
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.sampleShadingEnable  = VK_FALSE;

        // --- Color blending (opaque, no blend) ---
        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachment.blendEnable    = info.enableBlend;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable   = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments    = &blendAttachment;

        // --- No depth/stencil (design: no depth buffer in v1) ---
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable       = info.enableDepthTest;
        depthStencil.depthWriteEnable      = info.enableDepthWrite;
        depthStencil.depthBoundsTestEnable = info.enableDepthBoundsTest;
        depthStencil.stencilTestEnable     = info.enableStencilTest;

        // --- Assemble pipeline ---
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = static_cast<uint32_t>(stages.size());
        pipelineInfo.pStages             = stages.data();
        pipelineInfo.pVertexInputState   = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pDepthStencilState  = &depthStencil;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.pDynamicState       = &dynamicState;
        pipelineInfo.layout              = m_PipelineLayout;
        pipelineInfo.renderPass          = m_RenderPass;
        pipelineInfo.subpass             = 0;

        const VkResult res = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
        VULKAN_CHECK(res, "[VulkanGraphicsPipeline] vkCreateGraphicsPipelines failed : {}", static_cast<int>(res));
        return res == VK_SUCCESS;
    }

    // ---------------------------------------------------------------------------
    // BuildFramebuffers
    // ---------------------------------------------------------------------------
    bool VulkanGraphicsPipeline::BuildFramebuffers(
        const std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT>& imageViews,
        uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0 || imageViews[0] == VK_NULL_HANDLE)
        {
            return true;
        }

        m_Width  = width;
        m_Height = height;

        for (uint32_t i = 0; i < PIPELINE_BACKBUFFER_COUNT; ++i)
        {
            if (imageViews[i] == VK_NULL_HANDLE)
            {
                continue;
            }

            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass      = m_RenderPass;
            fbInfo.attachmentCount = 1;
            fbInfo.pAttachments    = &imageViews[i];
            fbInfo.width           = width;
            fbInfo.height          = height;
            fbInfo.layers          = 1;

            VkResult res = vkCreateFramebuffer(m_Device, &fbInfo, nullptr, &m_Framebuffers[i]);
            if (res != VK_SUCCESS)
            {
                LOG_ERROR("[VulkanGraphicsPipeline] Failed to create framebuffer [{}]: {}", i, static_cast<int>(res));
                return false;
            }
        }
        return true;
    }

    // ---------------------------------------------------------------------------
    // RecreateFramebuffers
    // ---------------------------------------------------------------------------
    bool VulkanGraphicsPipeline::RecreateFramebuffers(
        const std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT>& imageViews,
        uint32_t width, uint32_t height)
    {
        if (m_Device == VK_NULL_HANDLE) return false;
        DestroyFramebuffers();
        return BuildFramebuffers(imageViews, width, height);
    }

    // ---------------------------------------------------------------------------
    // Destroy
    // ---------------------------------------------------------------------------
    void VulkanGraphicsPipeline::DestroyFramebuffers()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);
            for (uint32_t i = 0; i < PIPELINE_BACKBUFFER_COUNT; ++i)
            {
                if (m_Framebuffers[i] != VK_NULL_HANDLE)
                {
                    vkDestroyFramebuffer(m_Device, m_Framebuffers[i], nullptr);
                    m_Framebuffers[i] = VK_NULL_HANDLE;
                }
            }
        }
    }


    void VulkanGraphicsPipeline::Destroy()
    {
        if (m_Device == VK_NULL_HANDLE) return;

        vkDeviceWaitIdle(m_Device);

        DestroyFramebuffers();

        if (m_Pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
            m_Pipeline = VK_NULL_HANDLE;
        }
        if (m_PipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }
        if (m_OwnsDescriptorSetLayouts)
        {
            for (auto& layout : m_DescriptorSetLayouts)
            {
                if (layout != VK_NULL_HANDLE)
                    vkDestroyDescriptorSetLayout(m_Device, layout, nullptr);
            }
        }
        m_DescriptorSetLayouts.clear();

        if (m_RenderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }
    }
}
