#pragma once

[[vk::binding(0)]] ConstantBuffer<UniformBuffer> ubo;
[[vk::binding(1)]] Sampler2D skybox;

struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
    float4 lightPos;
    float4 cameraPos;
};

struct VSInput {
    [[vk::location(0)]] float3 inPos : POSITION;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 localPos : TEXCOORD0;
};

static const float PI = 3.14159265358979f;

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.localPos = input.inPos;

    // Strip translation from view matrix — only rotation affects the skybox
    float4x4 rotView = ubo.view;
    rotView[0][3] = 0.0f;
    rotView[1][3] = 0.0f;
    rotView[2][3] = 0.0f;

    float4 pos = mul(ubo.proj, mul(rotView, float4(input.inPos, 1.0f)));
    // Set z = w so depth is always 1.0 (furthest possible)
    output.position = pos.xyww;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_TARGET {
    float3 dir = normalize(input.localPos);

    // Convert direction to equirectangular UV
    float u = atan2(dir.y, dir.x) / (2.0f * PI) + 0.5f;
    float v = asin(clamp(dir.z, -1.0f, 1.0f)) / PI + 0.5f;

    return skybox.Sample(float2(u, v));
}