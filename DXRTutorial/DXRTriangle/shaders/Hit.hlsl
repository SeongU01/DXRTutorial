#include "Common.hlsl"

// ���� �ϳ��� ��ġ�� ����
struct STriVertex
{
    float3 vertex;
    float4 color;
};


// �ﰢ�� ���� ������
StructuredBuffer<STriVertex> BTriVertex : register(t0);

// ���� ����� �ȼ����� �浹�� ���� �� �� ����Ǵ� ���̴�
[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    // �浹 ������ �ﰢ�� �� �� ��ġ
    float3 barycentrics =
        float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    // �浹�� �ﰢ�� ��ȣ
    uint vertId = 3 * PrimitiveIndex();
    float3 hitColor = BTriVertex[vertId + 0].color * barycentrics.x +
                      BTriVertex[vertId + 1].color * barycentrics.y +
                      BTriVertex[vertId + 2].color * barycentrics.z;
    // ����� �Ÿ��� ������ �����ܰ�� �ѱ��.
    payload.colorAndDistance = float4(hitColor, RayTCurrent());
}
