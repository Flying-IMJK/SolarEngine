#ifndef SKY_ATMOSPHERE_COMMON
#define SKY_ATMOSPHERE_COMMON

// Hillaire 2020 / UE 5.8 LUT parameterization and integration conventions.
// Planet-relative positions are km. Coefficients are km^-1; light uses engine linear units.
static const float AtmospherePi = 3.14159265358979323846;
static const float AtmosphereBoundaryOffset = 0.001;

struct SkyAtmosphereParameters
{
    float4 RadiiDensity;
    float4 RayleighScattering;
    float4 MieScattering;
    float4 MieAbsorption;
    float4 AbsorptionExtinction;
    float4 AbsorptionDensity;
    float4 GroundAlbedo;
};

struct SkyAtmosphereViewParameters
{
    float4x4 InvProjection;
    float4x4 InvView;
    float4 CameraPositionKm;
    float4 SunDirection;
    // RGB: unattenuated sunlight; W: height-fog contribution scale.
    float4 SunIlluminance;
    float4 SkyRight;
    float4 SkyForward;
    float4 SkyUp;
    float4 AerialPerspective;
};

struct AtmospherePreComputeData
{
    SkyAtmosphereParameters Atmosphere;
    SkyAtmosphereViewParameters View;
};

ConstantBuffer<AtmospherePreComputeData> data;
Texture2D<float4> TransmittanceLut;
Texture2D<float4> MultiScatteringLut;
SamplerState AtmosphereSampler;

struct AtmosphereMedium
{
    float3 Rayleigh;
    float3 Mie;
    float3 Scattering;
    float3 Extinction;
};

AtmosphereMedium SampleAtmosphereMedium(float3 position)
{
    float height = max(0.0, length(position) - data.Atmosphere.RadiiDensity.x);
    float rayleighDensity = exp(-height / data.Atmosphere.RadiiDensity.z);
    float mieDensity = exp(-height / data.Atmosphere.RadiiDensity.w);
    float ozoneDensity = saturate(1.0 - abs(height - data.Atmosphere.AbsorptionDensity.x) / data.Atmosphere.AbsorptionDensity.y);
    AtmosphereMedium medium;
    medium.Rayleigh = rayleighDensity * data.Atmosphere.RayleighScattering.rgb;
    medium.Mie = mieDensity * data.Atmosphere.MieScattering.rgb;
    medium.Scattering = medium.Rayleigh + medium.Mie;
    medium.Extinction = medium.Scattering + mieDensity * data.Atmosphere.MieAbsorption.rgb + ozoneDensity * data.Atmosphere.AbsorptionExtinction.rgb;
    return medium;
}

// Returns both intersections along a normalized ray, or (-1,-1) when missing.
float2 AtmosphereSphereIntersections(float3 origin, float3 direction, float radius)
{
    float b = dot(origin, direction);
    float c = dot(origin, origin) - radius * radius;
    float discriminant = b * b - c;
    if (discriminant < 0.0)
        return float2(-1.0, -1.0);
    float root = sqrt(discriminant);
    return float2(-b - root, -b + root);
}

float AtmosphereNearestIntersection(float3 origin, float3 direction, float radius)
{
    float2 intersections = AtmosphereSphereIntersections(origin, direction, radius);
    return intersections.x >= 0.0 ? intersections.x : intersections.y;
}

// Moves an exterior ray just inside the top boundary and shortens its remaining interval.
bool AtmosphereMoveToTop(inout float3 origin, float3 direction, inout float distance)
{
    float top = data.Atmosphere.RadiiDensity.y;
    if (length(origin) < top)
        return true;
    float entry = AtmosphereNearestIntersection(origin, direction, top);
    if (entry < 0.0 || entry >= distance)
        return false;
    float3 boundary = origin + entry * direction;
    origin = boundary - normalize(boundary) * AtmosphereBoundaryOffset;
    distance = max(0.0, distance - entry);
    return true;
}

// Bruneton 2017 mapping, as used by UE's Transmittance LUT.
void AtmosphereTransmittanceParameters(float2 uv, out float radius, out float mu)
{
    float bottom = data.Atmosphere.RadiiDensity.x;
    float top = data.Atmosphere.RadiiDensity.y;
    float h = sqrt(max(0.0, top * top - bottom * bottom));
    float rho = h * uv.y;
    radius = sqrt(rho * rho + bottom * bottom);
    float distance = lerp(top - radius, rho + h, uv.x);
    mu = distance == 0.0 ? 1.0 : clamp((h * h - rho * rho - distance * distance) / (2.0 * radius * distance), -1.0, 1.0);
}

float2 AtmosphereTransmittanceUv(float radius, float mu)
{
    float bottom = data.Atmosphere.RadiiDensity.x;
    float top = data.Atmosphere.RadiiDensity.y;
    radius = clamp(radius, bottom, top);
    float h = sqrt(max(0.0, top * top - bottom * bottom));
    float rho = sqrt(max(0.0, radius * radius - bottom * bottom));
    float discriminant = radius * radius * (mu * mu - 1.0) + top * top;
    float distance = max(0.0, -radius * mu + sqrt(max(0.0, discriminant)));
    float minimum = top - radius;
    return saturate(float2((distance - minimum) / max(1.e-6, rho + h - minimum), rho / h));
}

float3 AtmosphereTransmittance(float3 position, float3 lightDirection)
{
    float radius = length(position);
    return TransmittanceLut.SampleLevel(AtmosphereSampler, AtmosphereTransmittanceUv(radius, dot(position / radius, lightDirection)), 0).rgb;
}

float3 AtmosphereMultipleScattering(float3 position, float muSun)
{
    float height = (length(position) - data.Atmosphere.RadiiDensity.x) / (data.Atmosphere.RadiiDensity.y - data.Atmosphere.RadiiDensity.x);
    return MultiScatteringLut.SampleLevel(AtmosphereSampler, saturate(float2(muSun * 0.5 + 0.5, height)), 0).rgb;
}

// SkyView is a non-linear latitude/longitude map. The horizon lies at v=0.5.
float3 AtmosphereSkyViewDirection(float2 uv, float radius)
{
    uv = (uv - 0.5 / float2(192.0, 104.0)) / (1.0 - 1.0 / float2(192.0, 104.0));
    float bottom = data.Atmosphere.RadiiDensity.x;
    float beta = acos(clamp(sqrt(max(0.0, radius * radius - bottom * bottom)) / radius, 0.0, 1.0));
    float horizonAngle = AtmospherePi - beta;
    float angle;
    if (uv.y < 0.5)
    {
        float coordinate = 1.0 - 2.0 * uv.y;
        angle = horizonAngle * (1.0 - coordinate * coordinate);
    }
    else
    {
        float coordinate = 2.0 * uv.y - 1.0;
        angle = horizonAngle + beta * coordinate * coordinate;
    }
    float longitude = uv.x * 2.0 * AtmospherePi;
    return float3(sin(angle) * cos(longitude), sin(angle) * sin(longitude), cos(angle));
}

float2 AtmosphereSkyViewUv(float3 localDirection, float radius)
{
    float bottom = data.Atmosphere.RadiiDensity.x;
    float beta = acos(clamp(sqrt(max(0.0, radius * radius - bottom * bottom)) / radius, 0.0, 1.0));
    float horizonAngle = AtmospherePi - beta;
    float angle = acos(clamp(localDirection.z, -1.0, 1.0));
    float v = angle < horizonAngle ? 0.5 * (1.0 - sqrt(saturate(1.0 - angle / horizonAngle))) :
                                   0.5 + 0.5 * sqrt(saturate((angle - horizonAngle) / max(beta, 1.e-6)));
    float u = atan2(localDirection.y, localDirection.x) / (2.0 * AtmospherePi);
    if (u < 0.0)
        u += 1.0;
    return 0.5 / float2(192.0, 104.0) + float2(u, v) * (1.0 - 1.0 / float2(192.0, 104.0));
}

float3 AtmosphereLocalToWorld(float3 direction)
{
    return direction.x * data.View.SkyRight.xyz + direction.y * data.View.SkyForward.xyz + direction.z * data.View.SkyUp.xyz;
}

float3 AtmosphereWorldToLocal(float3 direction)
{
    return float3(dot(direction, data.View.SkyRight.xyz), dot(direction, data.View.SkyForward.xyz), dot(direction, data.View.SkyUp.xyz));
}

// AerialPerspective generation and application must use this same square-distance mapping.
float AtmosphereAerialDistance(float slice)
{
    float coordinate = (slice + 0.5) / 16.0;
    return coordinate * coordinate * data.View.AerialPerspective.y;
}

float AtmosphereAerialDepthUv(float distanceKm)
{
    return sqrt(saturate(distanceKm / data.View.AerialPerspective.y));
}

struct AtmosphereIntegrationResult
{
    float3 Luminance;
    float3 Transmittance;
    float3 MultiScatAsOne;
};

// variableSamples matches UE's distance-dependent count and squared segment spacing.
AtmosphereIntegrationResult IntegrateAtmosphere(float3 origin, float3 direction, float maximumDistance,
    float3 lightDirection, float3 lightIlluminance, float sampleCount, bool variableSamples, bool groundBounce, bool multipleScattering, bool isotropicPhase)
{
    AtmosphereIntegrationResult result;
    result.Luminance = 0.0;
    result.Transmittance = 1.0;
    result.MultiScatAsOne = 0.0;
    float groundDistance = AtmosphereNearestIntersection(origin, direction, data.Atmosphere.RadiiDensity.x);
    float topDistance = AtmosphereNearestIntersection(origin, direction, data.Atmosphere.RadiiDensity.y);
    if (topDistance < 0.0)
        return result;
    float distance = min(maximumDistance, topDistance);
    if (groundDistance >= 0.0)
        distance = min(distance, groundDistance);
    if (distance <= 0.0)
        return result;

    float mu = dot(direction, lightDirection);
    float uniformPhase = 1.0 / (4.0 * AtmospherePi);
    float rayleighPhase = isotropicPhase ? uniformPhase : (3.0 / (16.0 * AtmospherePi)) * (1.0 + mu * mu);
    float g = data.Atmosphere.MieScattering.w;
    float miePhase = isotropicPhase ? uniformPhase : (1.0 - g * g) / (4.0 * AtmospherePi * pow(max(1.e-6, 1.0 + g * g - 2.0 * g * mu), 1.5));
    float count = variableSamples ? lerp(4.0, 32.0, saturate(distance / 150.0)) : sampleCount;
    float countFloor = floor(count);
    float floorDistance = distance * countFloor / count;
    for (float i = 0.0; i < count; i += 1.0)
    {
        float start = i / countFloor;
        float end = (i + 1.0) / countFloor;
        float t0 = variableSamples ? floorDistance * start * start : distance * i / count;
        float t1 = variableSamples ? (end > 1.0 ? distance : floorDistance * end * end) : distance * (i + 1.0) / count;
        float dt = t1 - t0;
        float3 position = origin + direction * lerp(t0, t1, 0.3);
        AtmosphereMedium medium = SampleAtmosphereMedium(position);
        float3 stepTransmittance = exp(-medium.Extinction * dt);
        float3 up = normalize(position);
        float3 phaseScattering = medium.Rayleigh * rayleighPhase + medium.Mie * miePhase;
        // Offset the test sphere away from a surface origin to avoid self-shadowing.
        float planetShadow = AtmosphereNearestIntersection(position - up * AtmosphereBoundaryOffset, lightDirection, data.Atmosphere.RadiiDensity.x) < 0.0 ? 1.0 : 0.0;
        float3 source = planetShadow * AtmosphereTransmittance(position, lightDirection) * phaseScattering;
        if (multipleScattering)
            source += AtmosphereMultipleScattering(position, dot(up, lightDirection)) * medium.Scattering;
        source *= lightIlluminance;
        float3 safeExtinction = max(medium.Extinction, 1.e-9);
        float3 integrated = (source - source * stepTransmittance) / safeExtinction;
        result.Luminance += result.Transmittance * integrated;
        // UE 5.8 default MULTI_SCATTERING_POWER_SERIE=0 uses this finite-step estimate.
        result.MultiScatAsOne += result.Transmittance * medium.Scattering * dt;
        result.Transmittance *= stepTransmittance;
    }
    if (groundBounce && groundDistance >= 0.0 && distance == groundDistance)
    {
        float3 groundPosition = origin + groundDistance * direction;
        float3 normal = normalize(groundPosition);
        result.Luminance += result.Transmittance * lightIlluminance * AtmosphereTransmittance(groundPosition, lightDirection) *
                            saturate(dot(normal, lightDirection)) * data.Atmosphere.GroundAlbedo.rgb / AtmospherePi;
    }
    return result;
}

#endif
