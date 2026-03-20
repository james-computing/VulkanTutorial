struct VSInput {
    float2 position;
    float3 color;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

struct VSOutput {
    float4 sv_position : SV_Position;
    float3 color;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 vertexPosition = float4(input.position, 0.0, 1.0);
    output.sv_position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, vertexPosition)));
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target {
    float3 color = vertIn.color;
    return float4(color, 1.0);
}