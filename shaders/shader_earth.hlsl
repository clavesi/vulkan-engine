struct PushConstants { float4x4 model; uint objectId; };
[[vk::push_constant]] PushConstants push;

struct UniformBuffer {
    float4x4 view, proj;
    float4 lightPos;
    float4 cameraPos;
};
[[vk::binding(0)]] ConstantBuffer<UniformBuffer> ubo;
[[vk::binding(1)]] Sampler2D dayMap;
[[vk::binding(2)]] Sampler2D nightMap;
[[vk::binding(3)]] Sampler2D normalMap;
[[vk::binding(4)]] Sampler2D specularMap;
[[vk::binding(5)]] Sampler2D cloudsMap;

struct VSInput {
    [[vk::location(0)]] float3 inPos      : POSITION;
    [[vk::location(1)]] float3 inNormal   : NORMAL;
    [[vk::location(2)]] float3 inColor    : COLOR;
    [[vk::location(3)]] float2 inTexCoord : TEXCOORD;
};

struct VSOutput {
    float4 position  : SV_POSITION;
    float3 worldPos  : TEXCOORD0;
    float3 normal    : TEXCOORD1;
    float2 texCoord  : TEXCOORD2;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 worldPos = mul(push.model, float4(input.inPos, 1.0));
    output.worldPos = worldPos.xyz;
    output.position = mul(ubo.proj, mul(ubo.view, worldPos));
    // Transform normal to world space
    float3x3 normalMatrix = (float3x3)push.model;
    output.normal   = normalize(mul(normalMatrix, input.inNormal));
    output.texCoord = input.inTexCoord;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_TARGET {
    float3 normal   = normalize(input.normal);

    // Sample normal map and transform from [0,1] to [-1,1]
    float3 normalMapSample = normalMap.Sample(input.texCoord).rgb * 2.0 - 1.0;
    // Blend normal map with geometry normal — simple additive blend
    normal = normalize(normal + normalMapSample * 0.5);

    float3 lightDir = normalize(ubo.lightPos.xyz - input.worldPos);
    float3 viewDir  = normalize(ubo.cameraPos.xyz - input.worldPos);

    // Sun-facing factor: 1 = full day, -1 = full night
    float NdotL = dot(normal, lightDir);

    // Day/night blend — smooth transition zone around the terminator
    float dayFactor   = smoothstep(-0.1, 0.3, NdotL);
    float nightFactor = smoothstep(0.3, -0.1, NdotL);

    float3 dayColor   = dayMap.Sample(input.texCoord).rgb;
    float3 nightColor = nightMap.Sample(input.texCoord).rgb * 1.5; // boost night lights

    // Specular — oceans are shiny (white in specular map), land is not
    float specularMask = specularMap.Sample(input.texCoord).r;
    float3 halfDir     = normalize(lightDir + viewDir);
    float  spec        = pow(max(dot(normal, halfDir), 0.0), 64.0) * specularMask;
    float3 specular    = float3(spec, spec, spec) * max(NdotL, 0.0);

    // Clouds — sample alpha from cloud texture
    float4 cloudSample = cloudsMap.Sample(input.texCoord);
    float  cloudAlpha  = cloudSample.r; // clouds texture is greyscale

    // Compose: blend day/night, add specular, overlay clouds
    float3 surface = dayColor * dayFactor + nightColor * nightFactor;
    surface += specular;
    // Clouds are white on day side, dark on night side
    float3 cloudColor = float3(1.0, 1.0, 1.0) * dayFactor;
    surface = lerp(surface, cloudColor, cloudAlpha * 0.8);

    return float4(surface, 1.0);
}