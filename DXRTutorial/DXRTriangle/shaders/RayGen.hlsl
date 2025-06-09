#include "Common.hlsl"

RWTexture2D<float4> output : register(u0);

RaytracingAccelerationStructure sceneBVH : register(t0);
[shader("raygeneration")]
void RayGen()
{
    HitInfo payload;
    payload.colorAndDistance = float4(0, 0, 0, 0);

    //현재 픽셀의 위치
    uint2 launchIndex = DispatchRaysIndex().xy;
    // 화면 크기
    float2 dims = float2(DispatchRaysDimensions().xy);
    
    // 픽셀 위치를 정규화 하기 -1~1
    float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);
    
    RayDesc ray;
    ray.Origin = float3(d.x, -d.y, 1);// 카메라 위치
    ray.Direction = float3(0, 0, -1);// 광선 방향 (일단 고정)
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