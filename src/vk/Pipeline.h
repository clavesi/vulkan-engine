#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <string>

class Device;

class Pipeline {
public:
    // Creates a graphics pipeline from a single SPIR-V module containing
    // both vertex and fragment entry points ("vertMain" and "fragMain").
    // colorFormat is the swapchain's color attachment format.
    Pipeline(const Device &device, const std::string &shaderPath, vk::Format colorFormat);

    Pipeline(const Pipeline &) = delete;
    Pipeline &operator=(const Pipeline &) = delete;

    [[nodiscard]] const vk::raii::Pipeline &handle() const { return pipeline; }
    [[nodiscard]] const vk::raii::PipelineLayout &layout() const { return pipelineLayout; }

private:
    vk::raii::ShaderModule createShaderModule(
        const vk::raii::Device &device, const std::vector<char> &code
    ) const;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline pipeline = nullptr;
};
