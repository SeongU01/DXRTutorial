#include "Function.hlsli"

static const float3 cameraPosition = float3(0, 0, -2);
static const float4 backBufferColor = float4(0.4, 0.6, 0.2, 1.0);

RaytracingAccelerationStructure rtScene : register(t0);
RWTexture2D<float4> output : register(u0);

struct RayPayload
{
    float4 color;
};

[shader("raygeneration")]
void RayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();
    
    float2 coord = float2(launchIndex.xy);
    float2 dimension = float2(launchDim.xy);
    
    float2 d = ((coord / dimension) * 2.f - 1.f);
    float aspectRatio = dimension.x / dimension.y;
    
    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));
    ray.TMin = 0;
    ray.TMax = 100000;
    RayPayload payload;
    TraceRay(
    rtScene,
    0,
    0xFF,
    0, 0, 0,
    ray,
    payload
);
    output[launchIndex.xy] = linearToSrgb(payload.color);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = backBufferColor;
}

[shader("closesthit")]
void Closesthit(inout RayPayload payload,in BuiltInTriangleIntersectionAttributes attribs)
{
    float4 barycentries = float4(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y,1);
    
    const float4 r = float4(1, 0, 0, 1);
    const float4 g = float4(0, 1, 0, 1);
    const float4 b = float4(0, 0, 1, 1);
    
    payload.color = r * barycentries.x + g * barycentries.y + b * barycentries.z;
}