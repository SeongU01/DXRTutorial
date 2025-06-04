struct VSInput
{
    float4 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

PSInput vs_main(VSInput input)
{
    PSInput output = (PSInput) 0;
    output.position = input.position;
    output.color = input.color;
    return output;
}

float4 ps_main(PSInput input):SV_Target
{
    return input.color;
}