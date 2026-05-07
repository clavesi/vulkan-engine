struct VSInput {
    float3 inPos      : POSITION;
    float3 inNormal   : NORMAL;
    float3 inColor    : COLOR;
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
    uint     objectId;  // new — which object is being drawn
};

[[vk::push_constant]]
PushConstants push;

struct VSOutput {
    float4 pos : SV_Position;
};

struct FSOutput {
    uint objectId : SV_TARGET0;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 worldPos = mul(push.model, float4(input.inPos, 1.0));
    output.pos = mul(ubo.proj, mul(ubo.view, worldPos));
    return output;
}

[shader("fragment")]
FSOutput fragMain(VSOutput vertIn) {
    FSOutput output;
    output.objectId = push.objectId;
    return output;
}