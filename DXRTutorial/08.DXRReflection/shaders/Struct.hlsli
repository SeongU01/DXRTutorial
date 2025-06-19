static const float3 cameraPosition = float3(7.5, 0, -12.5);
static const float4 backBufferColor = float4(0.4, 0.6, 0.2, 1.0);
static const float3 lightPosition = float3(2, 2, -2.0);
static const float4 lightAmbient = float4(0.1, 0.1, 0.1, 1.0);
static const float4 lightDiffuse = float4(0.5,0.5,0.5, 1.0);
static const float4 Albedo = float4(1, 0, 1, 1);
static const float4 groundAlbedo = float4(1, 1, 1, 1);
static const float inShadowRadiance = 0.35;
static const uint maxRecursionDepth = 4;
static const float diffuseCoefficient = 0.9;
static const float specularCoefficient = 0.7;
static const float specularPower = 50;
static const float reflectionCoef = 0.9;

static const float3 cameraForward = float3(0, 0, 1);
static const float3 cameraRight= float3(1, 0, 0);
static const float3 cameraUp = float3(0, 1, 0);
static const float cameraFovY = 0.785f;

struct VertexPositionNormalTeangetTexture
{
    float3 position;
    float3 normal;
    float3 tangent;
    float2 uv;
};
