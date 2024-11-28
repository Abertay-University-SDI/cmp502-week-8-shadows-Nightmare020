
Texture2D shaderTexture : register(t0);
Texture2D depthMapTexture : register(t1);

SamplerState diffuseSampler  : register(s0);
SamplerState shadowSampler : register(s1);

cbuffer LightBuffer : register(b0)
{
	float4 ambient[2];
    float4 diffuse[2];
    float4 direction[2];
    float4 specularColour[2];
    float4 specularPower[2];
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
    float4 lightViewPos : TEXCOORD1;
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
    float shadowMapBias = 0.01f;
    
    float4 finalColour = float4(0.f, 0.f, 0.f, 1.f);
    float4 textureColour = shaderTexture.Sample(diffuseSampler, input.tex);

	// Calculate the projected texture coordinates.
    float2 pTexCoord = getProjectiveCoords(input.lightViewPos);
    
    // Calculate the view vector
    float3 viewVector = normalize(-input.position.xyz);
    
    for (int i = 0; i < 2; ++i)
    {
        // Lights accumulation
        float4 lightsAccumulation = float4(0.f, 0.f, 0.f, 1.f);
        
        // Shadow test. Is or isn't in shadow
        if (hasDepthData(pTexCoord))
        {
            // Has depth map data
            if (!isInShadow(depthMapTexture, pTexCoord, input.lightViewPos, shadowMapBias))
            {
                
                // is NOT in shadow, therefore light
                lightsAccumulation = calculateLighting(-direction[i].xyz, input.normal, diffuse[i]);
            
                // Add specular colour
                lightsAccumulation += calculateSpecular(direction[i].xyz, input.normal, viewVector, specularColour[i], specularPower[i].x);

            }
        }
       
        // Combine lightning with ambient and texture
        finalColour += saturate(lightsAccumulation + ambient[i]);  
    }
    
    return saturate(finalColour) * textureColour;
}