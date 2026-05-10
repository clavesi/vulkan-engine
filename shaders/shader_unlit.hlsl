struct VSInput {
    float3 inPos;
    float3 inNormal;
    float3 inColor;
    float2 inTexCoord;
};

struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
    float4 lightPos;
    float4 cameraPos;
};
ConstantBuffer<UniformBuffer> ubo;

struct PushConstants {
    float4x4 model;
    uint     objectId; // unused here, used in picking.hlsl
};
[[vk::push_constant]]
PushConstants push;

Sampler2D texture;

struct VSOutput {
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 worldPos = mul(push.model, float4(input.inPos, 1.0));
    output.pos = mul(ubo.proj, mul(ubo.view, worldPos));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    return texture.Sample(vertIn.fragTexCoord);
}