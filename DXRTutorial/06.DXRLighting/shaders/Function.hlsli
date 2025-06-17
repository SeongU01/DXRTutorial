#include "Struct.hlsli"

float4 linearToSrgb(float4 c)
{
    // Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
    float4 sq1 = sqrt(c);
    float4 sq2 = sqrt(sq1);
    float4 sq3 = sqrt(sq2);
    float4 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float CalculateDiffuseCoefficient(in float3 hitPos,in float3 incidentLightRay,in float3 noraml)
{
    float NDotL = saturate(dot(-incidentLightRay, noraml));
    return NDotL;
}

float CalculateSpecularCoefficient(in float3 hitPos,in float3 incidentLightRay,in float3 normal, in float specPower)
{
    float3 reflectLightRay = normalize(reflect(incidentLightRay, normal));
    return pow(saturate(dot(reflectLightRay, normalize(-WorldRayDirection()))), specPower);
}

float4 CalculatePhongLighting(
    in float4 albedo, in float3 normal,in float diffuseCoef=1.0,
    in float specularCoef=1.0,in float specPower = 50
)
{
    float3 hitPosition = HitWorldPosition();
    float3 incidentLightRay = normalize(hitPosition - lightPosition);
    
    float kd = CalculateDiffuseCoefficient(hitPosition, incidentLightRay, normal);
    float4 diffuseColor = diffuseCoef * kd * lightDiffuse * albedo;
    
    float4 specularColor = float4(0, 0, 0, 0);
    float ks = CalculateSpecularCoefficient(hitPosition, incidentLightRay, normal, specPower);
    specularColor = specularCoef * ks * lightSpecular;
    
    float4 ambientColorMin = lightAmbient - 0.15;
    float4 ambientColorMax = lightAmbient;
    
    float NdotL = saturate(dot(-incidentLightRay, normal));
    float4 ambientColor = albedo * lerp(ambientColorMin, ambientColorMax, NdotL);
    
    return ambientColor + diffuseColor + specularColor;
}
