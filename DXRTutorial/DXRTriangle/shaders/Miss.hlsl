#include "Common.hlsl"

// ray gen에서 생성한 광선을 검출했을 때 광선 검출이 되지않는 픽셀에 실행 되는 셰이더
[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);

    // 화면 높이에 따라 하늘색 그라데이션 효과
    float ramp = launchIndex.y / dims.y;
    payload.colorAndDistance = float4(0.0f, 0.2f, 0.7f - 0.3f * ramp, -1.0f);
}