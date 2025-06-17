#include "Function.hlsli"

RaytracingAccelerationStructure rtScene : register(t0);
RWTexture2D<float4> output : register(u0);

//ByteAddressBuffer Indices : register(t1);
StructuredBuffer<uint> Indices : register(t1);
StructuredBuffer<VertexPositionNormalTeangetTexture> vertices : register(t2);

struct RayPayload
{
    float4 color;
};

[shader("raygeneration")]
void RayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDimension = DispatchRaysDimensions();
    
    float2 coord = float2(launchIndex.xy);
    float2 dimension = float2(launchDimension.xy);
    
    float2 d = ((coord / dimension) * 2.0 - 1.0);
    float aspectRatio = dimension.x / dimension.y;
    
    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));
    ray.TMin = 0;
    ray.TMax = 100000;
    
    RayPayload payload;
    
    TraceRay(
    rtScene,
    0, 0xFF, 0, 0, 0, ray, payload
);
    output[launchIndex.xy] = linearToSrgb(payload.color);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = backBufferColor;
}

float3 HitAttribute(float3 vertexAttribute[3],BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

[shader("closesthit")]
void Closesthit(inout RayPayload payload,BuiltInTriangleIntersectionAttributes attribs)
{
    float3 hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    uint baseIndex = PrimitiveIndex() * 3;
    uint3 indices = uint3(
        Indices[baseIndex],
        Indices[baseIndex + 1],
        Indices[baseIndex + 2]
    );
    float3 vertexNormals[3] =
    {
        vertices[indices[0]].normal,
        vertices[indices[1]].normal,
        vertices[indices[2]].normal
    };
    float3 hitNormal = normalize(HitAttribute(vertexNormals, attribs));
    float4 phongColor = CalculatePhongLighting(
    Albedo, hitNormal, diffuseCoefficient, specularCoefficient, specularPower
);
    payload.color = phongColor;
}