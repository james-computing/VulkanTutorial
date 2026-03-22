struct VSInput {
    float3 position;
    float3 color;
    float2 textureCoord;
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
    float2 textureCoord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 vertexPosition = float4(input.position, 1.0);
    output.sv_position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, vertexPosition)));
    output.color = input.color;
    output.textureCoord = input.textureCoord;
    return output;
}

Sampler2D texture;

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target {
    //float3 color = vertIn.color;
    //return float4(color, 1.0);

    return texture.Sample(vertIn.textureCoord);
}