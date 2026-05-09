struct VSInput {
    float3 inPos    : POSITION;
    float3 inNormal : NORMAL;
    float3 inColor  : COLOR;
    float2 inTexCoord : TEXCOORD;
};

struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
    float4   lightPos;
    float4   cameraPos;
};
ConstantBuffer<UniformBuffer> ubo;

struct PushConstants {
    float4x4 model;
    uint     objectId;
};
[[vk::push_constant]]
PushConstants push;

struct VSOutput {
    float4 pos : SV_Position;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;

    // Extract scale from model matrix (length of first column)
    float objectScale = length(float3(push.model[0][0], push.model[1][0], push.model[2][0]));

    // Expand by a fixed screen-space-ish amount regardless of object size
    float expandAmount = 0.13 / max(objectScale, 0.001);

    float3 expandedPos = input.inPos + input.inNormal * expandAmount;
    float4 worldPos = mul(push.model, float4(expandedPos, 1.0));
    output.pos = mul(ubo.proj, mul(ubo.view, worldPos));
    return output;
}
[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    return float4(1.0, 0.65, 0.0, 1.0);
}