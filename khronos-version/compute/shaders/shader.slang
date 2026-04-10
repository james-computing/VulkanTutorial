struct VSInput {
    float2 position;
    float3 color;
};

struct VSOutput {
    float4 sv_position : SV_Position;
    float sv_pointSize : SV_PointSize;
    float3 color;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.sv_position = float4(input.position, 0.0, 1.0);
    output.color = input.color;
    output.sv_pointSize = 10;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target {
    float4 color = float4(vertIn.color, 1.0);
    return color;
}