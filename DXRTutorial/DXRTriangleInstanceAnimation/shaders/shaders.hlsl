cbuffer CameraParams : register(b0)
{
    float4x4 view;
    float4x4 projection;
}

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
    float4 pos = input.position;
    pos = mul(view, pos);
    pos = mul(projection, pos);
    output.position = pos;
    output.color = input.color;
    return output;
}

float4 ps_main(PSInput input):SV_Target
{
    return input.color;
}