#pragma once

#include "vk/Pipeline.h"
#include "Vertex.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace PipelineSpecs {
    inline PipelineSpec makeMain(const std::string &shaderPath, const vk::Format colorFormat,
                                 const vk::Format depthFormat, const vk::SampleCountFlagBits samples) {
        const auto attrs = Vertex::getAttributeDescriptions();

        // Vertex shader reads MVP matrices from the UBO
        vk::DescriptorSetLayoutBinding uboBinding{
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        };

        // Fragment shader samples colors from the texture
        vk::DescriptorSetLayoutBinding samplerBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = colorFormat,
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = {attrs.begin(), attrs.end()},
            .descriptorBindings = {uboBinding, samplerBinding},
            .depthFormat = depthFormat,
            .samples = samples,
            .pushConstantSize = sizeof(glm::mat4) + sizeof(uint32_t),
        };
    }

    inline PipelineSpec makeUnlit(const std::string &shaderPath,
                                  const vk::Format colorFormat,
                                  const vk::Format depthFormat,
                                  const vk::SampleCountFlagBits samples) {
        // Same as lit spec but with only 3 attributes — no normal
        const auto attrs = Vertex::getAttributeDescriptions();

        vk::DescriptorSetLayoutBinding uboBinding{
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        };
        vk::DescriptorSetLayoutBinding samplerBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };

        // Only locations 0, 2, 3 — skip normal at location 1
        const std::vector<vk::VertexInputAttributeDescription> unlitAttrs = {
            attrs[0], // location 0: pos
            attrs[2], // location 2: color
            attrs[3], // location 3: texcoord
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = colorFormat,
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = unlitAttrs,
            .descriptorBindings = {uboBinding, samplerBinding},
            .depthFormat = depthFormat,
            .samples = samples,
            .pushConstantSize = sizeof(glm::mat4) + sizeof(uint32_t),
        };
    }

    inline PipelineSpec makePicking(
        const std::string &shaderPath,
        const vk::Format depthFormat,
        const vk::SampleCountFlagBits samples
    ) {
        const std::vector<vk::VertexInputAttributeDescription> pickingAttrs = {
            Vertex::getAttributeDescriptions()[0], // location 0: pos only
        };

        vk::DescriptorSetLayoutBinding uboBinding{
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        };
        vk::DescriptorSetLayoutBinding samplerBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = vk::Format::eR32Uint, // picking image format
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = pickingAttrs,
            .descriptorBindings = {uboBinding, samplerBinding},
            .depthFormat = depthFormat,
            .samples = vk::SampleCountFlagBits::e1,
            .pushConstantSize = sizeof(glm::mat4) + sizeof(uint32_t), // no MSAA for picking
            .colorAttachmentFormat = vk::Format::eR32Uint,
        };
    }

    inline PipelineSpec makeOutline(
        const std::string &shaderPath,
        const vk::Format colorFormat,
        const vk::Format depthFormat,
        const vk::SampleCountFlagBits samples
    ) {
        const auto attrs = Vertex::getAttributeDescriptions();
        const std::vector<vk::VertexInputAttributeDescription> outlineAttrs = {
            attrs[0], // location 0: pos
            attrs[1], // location 1: normal
        };

        vk::DescriptorSetLayoutBinding uboBinding{
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        };
        vk::DescriptorSetLayoutBinding samplerBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = colorFormat,
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = outlineAttrs,
            .descriptorBindings = {uboBinding, samplerBinding},
            .depthFormat = depthFormat,
            .samples = samples,
            .pushConstantSize = sizeof(glm::mat4) + sizeof(uint32_t),
            .cullMode = vk::CullModeFlagBits::eFront,
            .depthTestEnable = true,
            .depthWriteEnable = false,
            .depthCompareOp = vk::CompareOp::eLess,
        };
    }

    inline PipelineSpec makeOrbit(
        const std::string &shaderPath,
        const vk::Format colorFormat,
        const vk::Format depthFormat,
        const vk::SampleCountFlagBits samples
    ) {
        const std::vector<vk::VertexInputAttributeDescription> orbitAttrs = {
            Vertex::getAttributeDescriptions()[0], // pos only
        };

        vk::DescriptorSetLayoutBinding uboBinding{
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = colorFormat,
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = orbitAttrs,
            .descriptorBindings = {uboBinding},
            .depthFormat = depthFormat,
            .samples = samples,
            .pushConstantSize = sizeof(glm::mat4) + sizeof(glm::vec3),
            .depthWriteEnable = false,
            .topology = vk::PrimitiveTopology::eLineStrip,
            .blendEnable = true
        };
    }

    inline PipelineSpec makeSkybox(
        const std::string &shaderPath,
        const vk::Format colorFormat,
        const vk::Format depthFormat,
        const vk::SampleCountFlagBits samples
    ) {
        const std::vector<vk::VertexInputAttributeDescription> skyboxAttrs = {
            Vertex::getAttributeDescriptions()[0], // pos only
        };

        vk::DescriptorSetLayoutBinding uboBinding{
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        };
        vk::DescriptorSetLayoutBinding samplerBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = colorFormat,
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = skyboxAttrs,
            .descriptorBindings = {uboBinding, samplerBinding},
            .depthFormat = depthFormat,
            .samples = samples,
            .pushConstantSize = 0,
            .cullMode = vk::CullModeFlagBits::eFront,
            .depthTestEnable = false,
            .depthWriteEnable = false,
        };
    }

    inline PipelineSpec makeEarth(const std::string &shaderPath, vk::Format colorFormat,
                                  vk::Format depthFormat, vk::SampleCountFlagBits samples) {
        const auto attrs = Vertex::getAttributeDescriptions();
        // Skip location 2 (color) — Earth shader doesn't use it
        std::vector<vk::VertexInputAttributeDescription> earthAttrs = {
            attrs[0], // location 0: pos
            attrs[1], // location 1: normal
            attrs[3], // location 3: texcoord
        };

        vk::DescriptorSetLayoutBinding uboBinding{
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr
        };

        // 5 texture bindings: day, night, normal, specular, clouds
        auto makeSampler = [](uint32_t binding) {
            return vk::DescriptorSetLayoutBinding{
                .binding = binding,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment
            };
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = colorFormat,
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = earthAttrs,
            .descriptorBindings = {
                uboBinding, makeSampler(1), makeSampler(2),
                makeSampler(3), makeSampler(4), makeSampler(5)
            },
            .depthFormat = depthFormat,
            .samples = samples,
            .pushConstantSize = sizeof(glm::mat4) + sizeof(uint32_t),
        };
    }
} // namespace PipelineSpecs
