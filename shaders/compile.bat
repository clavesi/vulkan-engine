:: Run this batch file to compile the shader.slang file into a SPIR-V binary.
:: Make sure you have the Vulkan SDK installed and that the path to slangc.exe is correct in the command below.
C:/VulkanSDK/1.4.341.1/bin/slangc.exe shader.slang -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o slang.spv