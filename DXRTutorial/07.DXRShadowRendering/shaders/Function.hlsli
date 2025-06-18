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

float4 CalculatePhongLighting(in float4 albedo, in float3 normal,in bool isInShadow,
                              in float diffuseCoef=1,in float specularCoef=1,in float specularPower=50)
{
    float3 hitPosition = HitWorldPosition();
    float shadowfactor = isInShadow ? inShadowRadiance : 1;
    float3 incidentLightRay = normalize(hitPosition - lightPosition);
    
    float kd = CalculateDiffuseCoefficient(hitPosition, incidentLightRay, normal);
    float4 diffuseColor = shadowfactor * diffuseCoef * kd * lightDiffuse * albedo;
    float4 specularColor = float4(0, 0, 0, 0);
    if (!isInShadow)
    {
        float4 lightSpecularColor = float4(1, 1, 1, 1);
        float4 ks = CalculateSpecularCoefficient(hitPosition, incidentLightRay, normal, specularPower);
        specularColor = specularCoef * ks * lightSpecularColor;
    }
    float4 ambientColor = lightAmbient;
    float4 ambientColorMin = lightAmbient - 0.15;
    float4 ambientColorMax = lightAmbient;
    float a = 1 - saturate(dot(normal, float3(0, -1, 0)));
    ambientColor = albedo * lerp(ambientColorMin, ambientColorMax, a);
    
    return ambientColor + diffuseColor + specularColor;
}