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

// Push constant block — updated per draw call, not per frame
struct PushConstants {
    float4x4 model;
    uint     objectId; // unused here, used in picking.hlsl
};
[[vk::push_constant]]
PushConstants push;

Sampler2D texture;

struct VSOutput {
    float4 pos : SV_Position;
    float3 fragNormal;
    float3 fragColor;
    float2 fragTexCoord;
    float3 fragWorldPos;   // world-space position for lighting
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;

    float4 worldPos = mul(push.model, float4(input.inPos, 1.0));
    output.pos = mul(ubo.proj, mul(ubo.view, worldPos));
    output.fragWorldPos = worldPos.xyz;

    // Transform normal into world space — use inverse transpose for correctness
    // with non-uniform scale. For uniform scale, mul(model, normal) is fine.
    float3x3 normalMatrix = (float3x3)push.model;
    output.fragNormal = normalize(mul(normalMatrix, input.inNormal));

    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    float3 normal    = normalize(vertIn.fragNormal);
    float3 lightDir  = normalize(ubo.lightPos.xyz - vertIn.fragWorldPos);
    float3 viewDir   = normalize(ubo.cameraPos.xyz - vertIn.fragWorldPos);
    float3 halfway   = normalize(lightDir + viewDir);

    // Ambient — base light so nothing is fully black
    float3 ambient = 0.1 * vertIn.fragColor;

    // Diffuse — brightness from angle between normal and light
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * vertIn.fragColor;

    // Specular — highlight from halfway vector
    float spec = pow(max(dot(normal, halfway), 0.0), 32.0);
    float3 specular = 0.3 * spec * float3(1.0, 1.0, 1.0);

    // Sample texture and modulate by lighting
    float4 texColor = texture.Sample(vertIn.fragTexCoord);
    float3 lit = (ambient + diffuse + specular) * texColor.rgb;

    return float4(lit, texColor.a);
}