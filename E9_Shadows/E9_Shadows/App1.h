// Application.h
#ifndef _APP1_H
#define _APP1_H

// Includes
#include "DXF.h"	// include dxframework
#include "TextureShader.h"
#include "ShadowShader.h"
#include "DepthShader.h"

class App1 : public BaseApplication
{
public:

	App1();
	~App1();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	bool frame();

protected:
	bool render();
	void depthPass();
	void shadowMapDebugger();
	void finalPass();
	void gui();

private:
	TextureShader* teapotTextureShader;
	TextureShader* floorTextureShader;
	PlaneMesh* planeMesh;
	CubeMesh* cubeMesh;
	SphereMesh* sphereMesh;
	SphereMesh* sunlightMesh;
	OrthoMesh* orthoMesh;
	OrthoMesh* shadowMapDebugOrthoMesh;
	RenderTexture* shadowMapDebugTexture;

	// Add a new camera for the shadow map debug
	Camera* rtTDebugCamera;

	Light* lights[2];
	AModel* model;
	ShadowShader* shadowShader;
	DepthShader* depthShader;
	TextureShader* textureShader;

	ShadowMap* shadowMap;

	// Variables for the purpose of tracking the meshes animations
	float teapotRotation = 0.0f; // Tracks the teapot's rotation
	float teapotRotationSpeed = 90.0f; // Adjust the speed of the teapot rotation
	float cubePosition = 0.0f;   // Tracks the cube's forward position
	float cubeTranslationSpeed = 5.0f; // Adjust the speed at which the cube translates
	float spherePosition = 0.0f; // Tracks the sphere's forward position
	float sphereRotation = 0.0f; // Tracks the sphere's rotation while moving forward
	float sphereTranslationSpeed = 10.0f; // Adjust the speed at which the sphere translates
	float sphereRotationSpeed = 360.0f; // Adjust the speed at which the sphere rotates

	// Light manipulation variables
	bool usePerspectiveProjection = false; // Default to orthographic
	XMFLOAT3 lightDirections[2];
	XMFLOAT3 lightPositions[2];
	float lightAmbientIntensities[2];
	float lightDiffuseIntensities[2];
	XMFLOAT4 lightSpecularColours[2];
	float lightSpecularPowers[2];
};

#endif