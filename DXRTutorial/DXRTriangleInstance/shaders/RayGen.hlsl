#include "Common.hlsl"

RWTexture2D<float4> output : register(u0);

RaytracingAccelerationStructure sceneBVH : register(t0);

cbuffer CameraParams : register(b0)
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewInverse;
    float4x4 projectionInverse;
}

[shader("raygeneration")]
void RayGen()
{
    HitInfo payload;
    payload.colorAndDistance = float4(0, 0, 0, 0);
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);

    RayDesc ray;
    ray.Origin = mul(viewInverse, float4(0, 0, 0, 1));
    float4 target = mul(projectionInverse, float4(d.x, -d.y, 1, 1));
    ray.Direction = mul(viewInverse, float4(target.xyz, 0));
    ray.TMin = 0;
    ray.TMax = 100000;
   
    TraceRay(
    sceneBVH,
    RAY_FLAG_NONE,
    0xFF,
    0, 
    0,
    0,
    ray,
    payload
);
    output[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);

}