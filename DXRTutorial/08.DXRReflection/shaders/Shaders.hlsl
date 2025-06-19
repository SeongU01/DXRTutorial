#include "Function.hlsli"

RaytracingAccelerationStructure rtScene : register(t0);
RWTexture2D<float4> output : register(u0);

StructuredBuffer<uint> Indices : register(t1);
StructuredBuffer<VertexPositionNormalTeangetTexture> vertices : register(t2);

struct RayPayload
{
    float4 color;
    uint recursionDepth;
};

struct ShadowPayload
{
    bool hit;
};

//[shader("raygeneration")]
//void RayGen()
//{
//    uint3 launchIndex = DispatchRaysIndex();
//    uint3 launchDimension = DispatchRaysDimensions();

//    float2 coord = float2(launchIndex.xy);
//    float2 dimension = float2(launchDimension.xy);
    
//    float2 d = ((coord / dimension) * 2.f - 1.f);
//    float aspectRatio = dimension.x / dimension.y;
    
//    RayDesc ray;
//    ray.Origin = cameraPosition;
//    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));
//    ray.TMin = 0;
//    ray.TMax = 100000;
    
//    RayPayload payload;
//    payload.recursionDepth = 0;
//    TraceRay(rtScene,
//    0, 0xFF, 0, 0, 0, ray, payload
//);
//    output[launchIndex.xy] = linearToSrgb(payload.color);

//}

[shader("raygeneration")]
void RayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 coord = float2(launchIndex.xy);
    float2 dim = float2(launchDim.xy);

    float2 ndc = (coord / dim) * 2.f - 1.f;
    ndc.y = -ndc.y;

    float aspect = dim.x / dim.y;
    float scaleY = tan(cameraFovY * 0.5f);
    float scaleX = aspect * scaleY;

    float3 rayDir =
        normalize(ndc.x * scaleX * cameraRight +
                  ndc.y * scaleY * cameraUp +
                  cameraForward);
    
    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = rayDir;
    ray.TMin = 0;
    ray.TMax = 100000;
    
    RayPayload payload;
    payload.recursionDepth = 0;
    TraceRay(rtScene,
    0, 0xFF, 0, 0, 0, ray, payload
);
    output[launchIndex.xy] = linearToSrgb(payload.color);

}

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.color = backBufferColor;
}

float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}
[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 hitPosition = HitWorldPosition();
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
    float3 hitNormal = (InstanceID() == 0) ? float3(0, 1, 0) : HitAttribute(vertexNormals, attribs);
     
    float4 color;
    float4 diffuseColor = (InstanceID() == 0) ? groundAlbedo : Albedo;
    if (payload.recursionDepth < maxRecursionDepth)
    {
        RayDesc shadowRay;
        shadowRay.Origin = hitPosition;
        shadowRay.Direction = normalize(lightPosition - shadowRay.Origin);
        shadowRay.TMin = 0.01;
        shadowRay.TMax = 100000;
        ShadowPayload shadowPayload;
        TraceRay(
        rtScene, 0, 0xFF, 1, 0, 1, shadowRay, shadowPayload);
        
        RayDesc reflectionRay;
        reflectionRay.Origin = hitPosition;
        reflectionRay.Direction = reflect(WorldRayDirection(), hitNormal);
        reflectionRay.TMin = 0.01;
        reflectionRay.TMax = 100000;
    
        RayPayload reflectPayload;
        reflectPayload.recursionDepth = payload.recursionDepth + 1;
        TraceRay(
    rtScene, 0, 0xFF, 0, 0, 0, reflectionRay, reflectPayload);
        float4 reflectionColor = reflectPayload.color;
        float3 fresnelR = FresnelReflectanceSchlick(WorldRayDirection(), hitNormal, diffuseColor.xyz);
        float4 reflectedColor = reflectedColor = reflectionCoef * float4(fresnelR, 1) * reflectionColor;
        
        float4 phongColor = CalculatePhongLighting(diffuseColor, hitNormal, shadowPayload.hit,
        diffuseCoefficient, specularCoefficient, specularPower);
        color = phongColor + reflectedColor;
    }
    else
    {
        color = CalculatePhongLighting(diffuseColor, hitNormal, false,
        diffuseCoefficient, specularCoefficient, specularPower);
    }
    
    float t = RayTCurrent();
    color = lerp(color, backBufferColor, 1.0 - exp(-0.000002 * pow(t, 3)));
    payload.color = float4(color.xyz, 1);
};

//[shader("closesthit")]
//void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes attribs)
//{
//    float3 hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
//    uint baseIndex = PrimitiveIndex() * 3;
//    uint3 indices = uint3(
//        Indices[baseIndex],
//        Indices[baseIndex + 1],
//        Indices[baseIndex + 2]
//    );
//    float3 vertexNormals[3] =
//    {
//        vertices[indices[0]].normal,
//        vertices[indices[1]].normal,
//        vertices[indices[2]].normal
//    };
//    float3 hitNormal = (InstanceID() == 0) ? float3(0, 1, 0) : HitAttribute(vertexNormals, attribs);
    
//    RayDesc shadowRay;
//    shadowRay.Origin = hitPosition;
//    shadowRay.Direction = normalize(lightPosition - shadowRay.Origin);
//    shadowRay.TMin = 0.01;
//    shadowRay.TMax = 100000;
//    ShadowPayload shadowPayload;
//    TraceRay(
//    rtScene, 0, 0xFF, 1, 0, 1, shadowRay, shadowPayload
//);
    
//    float4 diffuseColor = (InstanceID() == 0) ? groundAlbedo : Albedo;
//    float4 phongColor = CalculatePhongLighting(diffuseColor, hitNormal, shadowPayload.hit,
//                        diffuseCoefficient, specularCoefficient, specularPower);
//    float4 color = phongColor;
    
//    float t = RayTCurrent();
//    color = lerp(color, backBufferColor, 1.0 - exp(-0.000002 * t * t * t));
    
//    payload.color = color;
//}


[shader("miss")]
void ShadowMiss(inout ShadowPayload payload)
{
    payload.hit = false;
}