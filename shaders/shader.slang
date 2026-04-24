struct VSInput {
    float2 inPosition;
    float3 inColor;
};

struct VSOutput {
    float4 pos : SV_Position;
    float3 color;
};

// invoked for every vertex
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = float4(input.inPosition, 0.0, 1.0);
    output.color = input.inColor;
    return output;
}

// invoked for every fragment formed by the vertex shader
[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    return float4(vertIn.color, 1.0);
}