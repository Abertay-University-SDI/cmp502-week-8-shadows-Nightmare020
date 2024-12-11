
Texture2D shaderTexture : register(t0);
Texture2D depthMapTexture2 : register(t1);
Texture2D depthMapTexture1 : register(t2);

SamplerState diffuseSampler : register(s0);
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
    float4 lightViewPos[2] : TEXCOORD1;
    float3 viewVector : TEXCOORD3;
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
    float shadowMapBias = 0.01;
    float4 colour = float4(0.f, 0.f, 0.f, 1.f);
    float4 textureColour = shaderTexture.Sample(diffuseSampler, input.tex);
        
        // Calculate the projected texture coordinates.
    float2 pTexCoord1 = getProjectiveCoords(input.lightViewPos[0]);
    float2 pTexCoord2 = getProjectiveCoords(input.lightViewPos[1]);
	
    float3 normalizedDir1 = normalize(-direction[0].xyz);
    float3 normalizedDir2 = normalize(-direction[1].xyz);
        
    // Shadow test. Is or isn't in shadow
    if (hasDepthData(pTexCoord1) || hasDepthData(pTexCoord2))
    {
        // Initial shadow factor
        float finalShadow = 1.0f;
        
        // Check shadow for the first light
        if (isInShadow(depthMapTexture1, pTexCoord1, input.lightViewPos[0], shadowMapBias))
        {
            finalShadow *= 0.0f;
        }
        
        if (isInShadow(depthMapTexture2, pTexCoord2, input.lightViewPos[1], shadowMapBias))
        {
            finalShadow *= 0.0f;
        }
        
        // Has depth map data
        if (finalShadow > 0.0f)
        {
            // is NOT in shadow, therefore light
            float4 directionalLightColour1 = ambient[0] + calculateLighting(normalizedDir1, input.normal, diffuse[0]);
            float4 directionalLightColour2 = ambient[1] + calculateLighting(normalizedDir2, input.normal, diffuse[1]);
            
            // Calculate specular colour
            float4 specularColour1 = calculateSpecular(normalizedDir1, input.normal, input.viewVector, specularColour[0], specularPower[0].x);
            float4 specularColour2 = calculateSpecular(normalizedDir2, input.normal, input.viewVector, specularColour[1], specularPower[1].x);
                
            // Add specular colour
            directionalLightColour1 += specularColour1;
            directionalLightColour2 += specularColour2;
                
            // Accumulate the light colour from this directional light
            colour = directionalLightColour1 + directionalLightColour2;
        }
    }
    
    return saturate(colour) * textureColour;
}