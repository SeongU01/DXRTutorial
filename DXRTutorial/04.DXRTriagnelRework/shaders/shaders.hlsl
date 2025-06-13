RaytracingAccelerationStructure rtScene : register(t0);
RWTexture2D<float4> output : register(u0);

cbuffer PerFrame : register(b0)
{
    float3 A;
    float3 B;
    float3 C;
}

float3 linearToSrgb(float3 c)
{
    // Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
    float3 sq1 = sqrt(c);
    float3 sq2 = sqrt(sq1);
    float3 sq3 = sqrt(sq2);
    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

// payload �� ������ ���� �߰� ����� ��� ����� �����ϱ� ���� �����̳� �̴�.

struct ShadowPayload
{
    bool hit;
};

struct RayPayload
{
    float3 color;
};

//float2 ndc = (coord / dims) * 2.0f - 1.0f;

//    // aspect ratio
//float aspectRatio = dims.x / dims.y;

//    // FOV ���� (tan(fovY * 0.5))
//float fovScale = tan(fovY * 0.5f);

//    // NDC���� ���� �������� ��ȯ
//float3 rayDir = normalize(
//        ndc.x * aspectRatio * fovScale * cameraRight +
//        -ndc.y * fovScale * cameraUp +
//        cameraForward);

[shader("raygeneration")]
void RayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();
    float2 coord = float2(launchIndex.xy);
    // �ػ�
    float2 dims = float2(launchDim.xy);
    // ȭ��󿡼� ī�޶� �� ������ x/y ����
    float2 d = ((coord / dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;
    
    RayDesc ray;
    // ������ ��ġ. ���� ī�޶� ��⿡ ī�޶� ��ġ��.
    ray.Origin = float3(0, 0, -2);
    // �������� ��� ���� ����
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));
    // ������ �˻��� �ּ�/�ִ� �Ÿ�
    ray.TMin = 0;
    ray.TMax = 100000;
    
    RayPayload payload;
    TraceRay(
    rtScene, // ���� ����ü
    0, // Ray Flags
    0xFF, // Instance Mask(instance filtering mask, ���� 0xFF�� �����-> ��� �ν��Ͻ� �˻�)
    0, // Ray Contribution to Hit Group Index(������ ������ �� ���)
    2, // Multiplier for Geometry Contribution to Hit Group Index
    0, // Miss Shader Index
    ray, // RayDesc ����ü
    payload // ����� ���� payload
);
    float3 col = linearToSrgb(payload.color);
    output[launchIndex.xy] = float4(col, 1);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float3(0.4, 0.6, 0.2);
}

[shader("closesthit")]                       //������ �ﰢ���� ��� ��ġ�� �����ߴ����� ��Ÿ���� �ٸ���Ʈ�� ��ǥ
void TriangleChs(inout RayPayload payload,in BuiltInTriangleIntersectionAttributes attribs)
{
    //�� = 1 - u - v, �� = u, �� = v ������ ǥ����.
    float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, 
    attribs.barycentrics.x, attribs.barycentrics.y);
    payload.color = A * barycentrics.x + B * barycentrics.y + C * barycentrics.z;
}

[shader("closesthit")]
void PlaneChs(inout RayPayload payload,in BuiltInTriangleIntersectionAttributes attribs)
{
    float hitT = RayTCurrent();
    float3 rayDirW = WorldRayDirection();
    float3 rayOriginW = WorldRayOrigin();
    
    float3 posW = rayOriginW + hitT * rayDirW;
    
    RayDesc ray;
    ray.Origin = posW;
    ray.Direction = normalize(float3(0.5, 0.5, -0.5));
    ray.TMin = 0.01;
    ray.TMax = 100000;
    
    ShadowPayload shadowPayload;
    
    TraceRay(rtScene,0, 0xFF, 1, 0, 1, ray, shadowPayload);

    float facotr = shadowPayload.hit ? 0.1 : 1.0;
    payload.color = float4(0.9f, 0.9f, 0.9f, 1.f) * facotr;
}

[shader("closesthit")]
void ShadowChs(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.hit = true;
}

[shader("miss")]
void ShadowMiss(inout ShadowPayload payload)
{
    payload.hit = false;
}