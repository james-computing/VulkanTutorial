struct VSInput {
    float2 position;
    float3 color;
};

struct VSOutput {
    float4 sv_position : SV_Position;
    float3 color;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.sv_position = float4(input.position, 0.0, 1.0);
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target {
    float3 color = vertIn.color;
    return float4(color, 1.0);
}