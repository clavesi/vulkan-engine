struct PushConstants {
    float4x4 model;
    float3 color;
};
[[vk::push_constant]] PushConstants push;

struct UniformBuffer { float4x4 view, proj; float4 lightPos, cameraPos; };
[[vk::binding(0)]] ConstantBuffer<UniformBuffer> ubo;

struct VSInput {
    [[vk::location(0)]] float3 inPos : POSITION;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.position = mul(ubo.proj, mul(ubo.view, mul(push.model, float4(input.inPos, 1.0))));
    return output;
}
[shader("fragment")]
float4 fragMain(VSOutput input) : SV_TARGET {
    return float4(push.color, 0.2f); // slightly transparent
}