
Texture2D shaderTexture : register(t0);
Texture2D depthMapTexture : register(t1);

SamplerState diffuseSampler  : register(s0);
SamplerState shadowSampler : register(s1);

cbuffer LightBuffer : register(b0)
{
	float4 ambient;
	float4 diffuse;
	float3 direction;
    float padding1;
    float4 specularColour;
    float specularPower;
    float3 padding2;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
    float4 lightViewPos : TEXCOORD1;
    float3 viewVector : TEXCOORD2;
};

// Calculate lighting intensity based on direction and normal. Combine with light colour.
float4 calculateLighting(float3 lightDirection, float3 normal, float4 diffuse)
{
    float intensity = saturate(dot(normal, lightDirection));
    float4 colour = saturate(diffuse * intensity);
    return colour;
}

float4 calculateSpecular(float3 lightDirection, float3 normal, float3 viewVector, float4 specularColour, float specularPower)
{
	// Blinn-phong specular calculation
    float3 halfway = normalize(lightDirection + viewVector);
    float specularIntensity = pow(max(dot(normal, halfway), 0.f), specularPower);
    return saturate(specularColour * specularIntensity);
}

// Is the gemoetry in our shadow map
bool hasDepthData(float2 uv)
{
    if (uv.x < 0.f || uv.x > 1.f || uv.y < 0.f || uv.y > 1.f)
    {
        return false;
    }
    return true;
}

bool isInShadow(Texture2D sMap, float2 uv, float4 lightViewPosition, float bias)
{
    // Sample the shadow map (get depth of geometry)
    float depthValue = sMap.Sample(shadowSampler, uv).r;
	// Calculate the depth from the light.
    float lightDepthValue = lightViewPosition.z / lightViewPosition.w;
    
    // Adjust depth bias for perspective projection (scale by distance from the light)
    lightDepthValue -= bias * (1.0f / lightViewPosition.w);

	// Compare the depth of the shadow map value and the depth of the light to determine whether to shadow or to light this pixel.
    if (lightDepthValue < depthValue)
    {
        return false;
    }
    return true;
}

float2 getProjectiveCoords(float4 lightViewPosition)
{
    // Calculate the projected texture coordinates.
    float2 projTex = lightViewPosition.xy / lightViewPosition.w;
    projTex *= float2(0.5, -0.5);
    projTex += float2(0.5f, 0.5f);
    return projTex;
}

float4 main(InputType input) : SV_TARGET
{
    float cosTheta = abs(dot(input.normal, -direction));
    float shadowMapBias = 0.005 / max(cosTheta, 0.1f);
    float4 colour = float4(0.f, 0.f, 0.f, 1.f);
    float4 specular = float4(0.f, 0.f, 0.f, 1.f);
    float4 textureColour = shaderTexture.Sample(diffuseSampler, input.tex);

	// Calculate the projected texture coordinates.
    float2 pTexCoord = getProjectiveCoords(input.lightViewPos);
	
    // Shadow test. Is or isn't in shadow
    if (hasDepthData(pTexCoord))
    {
        // Has depth map data
        if (!isInShadow(depthMapTexture, pTexCoord, input.lightViewPos, shadowMapBias))
        {
            // is NOT in shadow, therefore light
            colour = calculateLighting(-direction, input.normal, diffuse);
            
            // Add specular colour
            specular = calculateSpecular(-direction, input.normal, input.viewVector, specularColour, specularPower);
        }
    }
    
    // Combine lightning with ambient and texture
    colour = saturate(colour + ambient);
    return saturate(colour + specular) * textureColour;
}