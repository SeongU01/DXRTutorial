#include "Common.hlsl"

// 정점 하나에 위치와 색상
struct STriVertex
{
    float3 vertex;
    float4 color;
};


// 삼각형 정점 데이터
StructuredBuffer<STriVertex> BTriVertex : register(t0);

// 광선 검출시 픽셀에서 충돌이 검출 될 때 실행되는 셰이더
[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    // 충돌 지점의 삼각형 면 내 위치
    float3 barycentrics =
        float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    // 충돌한 삼각형 번호
    uint vertId = 3 * PrimitiveIndex();
    float3 hitColor = BTriVertex[vertId + 0].color * barycentrics.x +
                      BTriVertex[vertId + 1].color * barycentrics.y +
                      BTriVertex[vertId + 2].color * barycentrics.z;
    // 색상과 거리를 저장해 다음단계로 넘긴다.
    payload.colorAndDistance = float4(hitColor, RayTCurrent());
}
