#include "Pipeline.h"
#include "Device.h"
#include "io/FileIO.h"

Pipeline::Pipeline(const Device &device, const PipelineSpec &spec) {
    // Create a shader module - just thin wrappers around the shader bytecode.
    const auto code = io::readBinaryFile(spec.shaderPath);
    auto shaderModule = createShaderModule(device.logical(), code);

    // Assign shaders to a specific pipeline stage
    vk::PipelineShaderStageCreateInfo vertStage{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"
    };
    vk::PipelineShaderStageCreateInfo fragStage{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};


    // This struct describes the format of the vertex data to be passed to the vert shader
    // Vertex input comes from spec
    vk::PipelineVertexInputStateCreateInfo vertexInput{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &spec.bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(spec.attributeDescriptions.size()),
        .pVertexAttributeDescriptions = spec.attributeDescriptions.data()
    };

    // This struct describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled.
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList
    };

    // Make viewport and scissor states dynamic
    vk::PipelineViewportStateCreateInfo viewport{
        .viewportCount = 1,
        .scissorCount = 1
    };

    // Rasterizer struct which takes the geometry from the vert shader and turns it into fragments to be colored by frag shader.
    vk::PipelineRasterizationStateCreateInfo rasterizer{
        // True means fragments that are beyond the near and far planes ar clamped to it instead of discarding them.
        // Also requires setting a GPU feature. Sometimes used for shadow maps.
        .depthClampEnable = vk::False,
        // True means geometry never passes through the rasterizer stage
        .rasterizerDiscardEnable = vk::False,
        // Determines how fragments are generated for geometry. Any other mode than this requires enabling a GPU feature.
        .polygonMode = vk::PolygonMode::eFill,
        // Determines the type of face culling to use. Disabled, front, back, or both.
        // Was eClockwise. The Y-flip in the projection matrix reverses winding order, so what's now "front-facing" is CCW.
        .cullMode = vk::CullModeFlagBits::eBack,
        // Specifies the vertex order for faces to be front-facing.
        .frontFace = vk::FrontFace::eCounterClockwise,
        // Can alter depth values by constant or based on a fragment's slope. Sometimes used for shadow mapping.
        .depthBiasEnable = vk::False,
        // Thickness of lines in terms of number of fragments. Any higher than 1.0f requires enabling the `wideLines` GPU feature.
        .lineWidth = 1.0f
    };

    // This struct describes multisampling, one of the ways to perform antialiasing. For now, it's disabled.
    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False
    };

    // If using a depth and/or stencil buffer, setup the struct for it. Since we're not, just set to nullptr.
    vk::PipelineDepthStencilStateCreateInfo depthStencil = {};

    // After frag shader turns color, it needs to combine with color already in the framebuffer.
    // This is color blending. Either mix the old and new or combine the old and new using bitwise
    // We need two structs:
    // 1) The state contains the configuration per attached framebuffer
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    // 2) Contains global color blending settings
    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    // A limited amount of data can be changed without recreating the pipeline at draw time, such as viewport size and line width.
    // So we need to use this dynamic state and keep those properties in it.
    // With this, you now have to specify this data at draw time.
    const vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

    // Build the descriptor set layout describing the resources the shader uses
    vk::DescriptorSetLayoutCreateInfo descriptorLayoutInfo{
        .bindingCount = static_cast<uint32_t>(spec.descriptorBindings.size()),
        .pBindings = spec.descriptorBindings.data()
    };
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device.logical(), descriptorLayoutInfo);

    // Create final graphics pipeline
    // Pipeline layout references the descriptor set layout so the shader knows where to find its bound resources at draw time
    vk::PipelineLayoutCreateInfo layoutInfo{
        .setLayoutCount = 1,
        .pSetLayouts = &*descriptorSetLayout,
        .pushConstantRangeCount = 0,
    };
    pipelineLayout = vk::raii::PipelineLayout(device.logical(), layoutInfo);

    // To use dynamic rendering, we need to specify the formats of the attachments that will be used.
    vk::PipelineRenderingCreateInfo renderingInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &spec.colorFormat
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = &renderingInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewport,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = nullptr, // null since we're using dynamic rendering
    };

    pipeline = vk::raii::Pipeline(device.logical(), nullptr, pipelineInfo);
}

/*
  * Take buffer with the bytecode and return a ShaderModule
  */
vk::raii::ShaderModule Pipeline::createShaderModule(
    const vk::raii::Device &device, const std::vector<char> &code
) const {
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t *>(code.data())
    };
    return vk::raii::ShaderModule{device, createInfo};
}
