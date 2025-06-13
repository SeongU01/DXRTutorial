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

// payload 는 광선의 실행 중간 결과를 계산 결과로 전달하기 위한 컨테이너 이다.

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

//    // FOV 조정 (tan(fovY * 0.5))
//float fovScale = tan(fovY * 0.5f);

//    // NDC에서 실제 방향으로 변환
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
    // 해상도
    float2 dims = float2(launchDim.xy);
    // 화면상에서 카메라가 쏠 방향의 x/y 성분
    float2 d = ((coord / dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;
    
    RayDesc ray;
    // 광원의 위치. 보통 카메라가 쏘기에 카메라 위치임.
    ray.Origin = float3(0, 0, -2);
    // 광원에서 쏘는 빛의 방향
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));
    // 광선이 검사할 최소/최대 거리
    ray.TMin = 0;
    ray.TMax = 100000;
    
    RayPayload payload;
    TraceRay(
    rtScene, // 가속 구조체
    0, // Ray Flags
    0xFF, // Instance Mask(instance filtering mask, 보통 0xFF를 사용함-> 모든 인스턴스 검사)
    0, // Ray Contribution to Hit Group Index(광선을 정의할 때 사용)
    2, // Multiplier for Geometry Contribution to Hit Group Index
    0, // Miss Shader Index
    ray, // RayDesc 구조체
    payload // 사용자 정의 payload
);
    float3 col = linearToSrgb(payload.color);
    output[launchIndex.xy] = float4(col, 1);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = float3(0.4, 0.6, 0.2);
}

[shader("closesthit")]                       //광선이 삼각형의 어느 위치와 교차했는지를 나타내는 바리센트릭 좌표
void TriangleChs(inout RayPayload payload,in BuiltInTriangleIntersectionAttributes attribs)
{
    //α = 1 - u - v, β = u, γ = v 식으로 표현됨.
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