static const float3 cameraPosition = float3(0, 0, -2);
static const float4 backBufferColor = float4(0.4, 0.6, 0.2, 1.0);
static const float3 lightPosition = float3(0, 2, -2.0);
static const float4 lightAmbient = float4(0.2, 0.2, 0.2, 1.0);
static const float4 lightDiffuse = float4(1, 1, 1, 1.0);
static const float4 lightSpecular = float4(1, 1, 1, 1);
static const float4 Albedo = float4(1, 0, 1, 1);
static const float diffuseCoefficient = 0.9;
static const float specularCoefficient = 0.7;
static const float specularPower = 50;

struct VertexPositionNormalTeangetTexture
{
    float3 position;
    float3 normal;
    float3 tangent;
    float2 uv;
};
