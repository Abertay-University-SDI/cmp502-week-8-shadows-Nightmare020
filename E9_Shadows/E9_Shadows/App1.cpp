// Lab1.cpp
// Lab 1 example, simple coloured triangle mesh
#include "App1.h"

App1::App1()
{

}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	// Create Mesh object and shader object
	planeMesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext());
	model = new AModel(renderer->getDevice(), "res/teapot.obj");
	cubeMesh = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());
	sphereMesh = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	sunlightMesh = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	textureMgr->loadTexture(L"ceramic", L"res/ceramic.png");
	textureMgr->loadTexture(L"wood", L"res/wood.png");
	textureMgr->loadTexture(L"brick", L"res/brick1.dds");
	textureMgr->loadTexture(L"glass", L"res/glass.png");
	textureMgr->loadTexture(L"sun", L"res/sun.png");

	// initial shaders
	teapotTextureShader = new TextureShader(renderer->getDevice(), hwnd);
	floorTextureShader = new TextureShader(renderer->getDevice(), hwnd);
	depthShader = new DepthShader(renderer->getDevice(), hwnd);
	shadowShader = new ShadowShader(renderer->getDevice(), hwnd);
	textureShader = new TextureShader(renderer->getDevice(), hwnd);

	// Variables for defining shadow map
	int shadowmapWidth = 2048;
	int shadowmapHeight = 2048;
	int sceneWidth = 100;
	int sceneHeight = 100;

	// This is your shadow map
	shadowMap = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);

	// Initialize ortho meshes
	orthoMesh1 = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(), screenWidth / 3.7, screenHeight / 3.7, -screenWidth / 2.8, screenHeight / 2.8);
	orthoMesh2 = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(), screenWidth / 3.7, screenHeight / 3.7, screenWidth / 2.8, -screenHeight / 2.8);

	// Configure directional light
	light = new Light();
	light->setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	light->setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	light->setDirection(0.0f, -0.7f, 0.7f);
	light->setPosition(0.f, 15.f, -10.f);
	light->setSpecularColour(0.0f, 0.0f, 0.0f, 1.0f);
	light->setSpecularPower(0.0f);
	light->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);

	// Configure sphere light for debug
	lightDirection = light->getDirection();
	lightPosition = light->getPosition();
}

App1::~App1()
{
	// Run base application deconstructor
	BaseApplication::~BaseApplication();

	// Release the Direct3D object.
	if (planeMesh != nullptr)
	{
		delete planeMesh;
	}

	if (cubeMesh != nullptr)
	{
		delete cubeMesh;
	}

	if (sphereMesh != nullptr)
	{
		delete sphereMesh;
	}

	if (sunlightMesh != nullptr)
	{
		delete sunlightMesh;
	}

	if (model != nullptr)
	{
		delete model;
	}

	if (orthoMesh1 != nullptr)
	{
		delete orthoMesh1;
	}

	if (orthoMesh2 != nullptr)
	{
		delete orthoMesh2;
	}
}


bool App1::frame()
{
	bool result;

	result = BaseApplication::frame();
	if (!result)
	{
		return false;
	}
	
	// Get the time elapsed since last frame
	float deltaTime = timer->getTime();

	// Rotate teapot at any degrees (gui) per second
	teapotRotation += teapotRotationSpeed * deltaTime;
	if (teapotRotation >= 360.0f)
	{
		// Wrap around if we reached 360º
		teapotRotation -= 360.0f;
	}

	// Move cube forward at 2 units per second
	cubePosition += cubeTranslationSpeed * deltaTime;
	if (cubePosition > 9.0f) 
	{
		// Reset position after moving forward 10 units
		cubePosition = -4.0f;
	}

	// Move and rotate sphere at 3 units per second, and at 360 degrees per second
	spherePosition += sphereTranslationSpeed * deltaTime;
	sphereRotation += sphereRotationSpeed * deltaTime;
	if (spherePosition > 50.0f)
	{
		// Reset position after moving forward 10 units
		spherePosition = -10.0f;
	}

	// Render the graphics.
	result = render();
	if (!result)
	{
		return false;
	}

	return true;
}

bool App1::render()
{

	// Perform depth pass
	depthPass();
	// Render scene
	finalPass();

	return true;
}

void App1::depthPass()
{
	// Set the render target to be the render to texture.
	shadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

	// get the world, view, and projection matrices from the camera and d3d objects.
	light->generateViewMatrix();
	XMMATRIX lightViewMatrix = light->getViewMatrix();
	XMMATRIX lightProjectionMatrix = light->getOrthoMatrix();

	if (usePerspectiveProjection)
	{
		light->generateProjectionMatrix(0.1f, 100.f);
		lightProjectionMatrix = light->getProjectionMatrix();
	}

	XMMATRIX worldMatrix = renderer->getWorldMatrix();

	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);

	// Render floor
	planeMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), planeMesh->getIndexCount());

	worldMatrix = renderer->getWorldMatrix();
	XMMATRIX translationMatrix = XMMatrixTranslation(0.f, 4.f, 20.f);
	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX rotationMatrix = XMMatrixRotationY(XMConvertToRadians(teapotRotation));
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

	// Render model
	model->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), model->getIndexCount());

	// Render cube and sphere
	worldMatrix = renderer->getWorldMatrix();
	translationMatrix = XMMatrixTranslation(-3.f, 1.f, 5.f + cubePosition);
	scaleMatrix = XMMatrixScaling(5.f, 5.f, 5.f);
	worldMatrix = translationMatrix * scaleMatrix;
	cubeMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());

	worldMatrix = renderer->getWorldMatrix();
	translationMatrix = XMMatrixTranslation(20.f, 4.f, 15.f + spherePosition);
	scaleMatrix = XMMatrixScaling(4.f, 4.f, 4.f);
	rotationMatrix = XMMatrixRotationX(XMConvertToRadians(sphereRotation));
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
	sphereMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());

	// Render sun light sphere
	worldMatrix = renderer->getWorldMatrix();
	translationMatrix = XMMatrixTranslation(lightPosition.x, lightPosition.y, lightPosition.z);
	scaleMatrix = XMMatrixScaling(10.f, 10.f, 10.f);
	worldMatrix = scaleMatrix * translationMatrix;
	sunlightMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), sunlightMesh->getIndexCount());

	// Set back buffer as render target and reset view port.
	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();
}

void App1::finalPass()
{
	// Clear the scene. (default blue colour)
	renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);
	camera->update();

	// Get the world, view, projection, and ortho matrices from the camera and Direct3D objects.
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();
	XMMATRIX lightProjectionMatrix = light->getOrthoMatrix();

	if (usePerspectiveProjection)
	{
		light->generateProjectionMatrix(0.1f, 100.f);
		lightProjectionMatrix = light->getProjectionMatrix();
	}

	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);

	// Render floor
	planeMesh->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, 
		textureMgr->getTexture(L"wood"), camera, shadowMap->getDepthMapSRV(), light, lightProjectionMatrix);
	shadowShader->render(renderer->getDeviceContext(), planeMesh->getIndexCount());

	// Render model
	worldMatrix = renderer->getWorldMatrix();
	XMMATRIX translationMatrix = XMMatrixTranslation(0.f, 4.f, 20.f);
	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX rotationMatrix = XMMatrixRotationY(XMConvertToRadians(teapotRotation));
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
	model->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"ceramic"), 
		camera, shadowMap->getDepthMapSRV(), light, lightProjectionMatrix);
	shadowShader->render(renderer->getDeviceContext(), model->getIndexCount());

	// Render cube and sphere
	worldMatrix = renderer->getWorldMatrix();
	translationMatrix = XMMatrixTranslation(-3.f, 1.f, 5.f + cubePosition);
	scaleMatrix = XMMatrixScaling(5.f, 5.f, 5.f);
	worldMatrix =  translationMatrix * scaleMatrix;
	cubeMesh->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"brick"), 
		camera, shadowMap->getDepthMapSRV(), light, lightProjectionMatrix);
	shadowShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());

	worldMatrix = renderer->getWorldMatrix();
	translationMatrix = XMMatrixTranslation(20.f, 4.f, 15.f + spherePosition);
	scaleMatrix = XMMatrixScaling(4.f, 4.f, 4.f);
	rotationMatrix = XMMatrixRotationX(XMConvertToRadians(sphereRotation));
	worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
	sphereMesh->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"glass"), 
		camera, shadowMap->getDepthMapSRV(), light, lightProjectionMatrix);
	shadowShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());

	// Render sun light sphere
	worldMatrix = renderer->getWorldMatrix();
	translationMatrix = XMMatrixTranslation(lightPosition.x, lightPosition.y, lightPosition.z);
	scaleMatrix = XMMatrixScaling(10.f, 10.f, 10.f);
	worldMatrix = scaleMatrix * translationMatrix;
	sunlightMesh->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"sun"), 
		camera, shadowMap->getDepthMapSRV(), light, lightProjectionMatrix);
	shadowShader->render(renderer->getDeviceContext(), sunlightMesh->getIndexCount());

	// Render the shadow map ortho mesh
	renderer->setZBuffer(false);

	// Get ortho matrices
	worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();
	XMMATRIX orthoMatrix = renderer->getOrthoMatrix();

	orthoMesh1->sendData(renderer->getDeviceContext());
	textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, shadowMap->getDepthMapSRV());
	textureShader->render(renderer->getDeviceContext(), orthoMesh1->getIndexCount());

	orthoMesh2->sendData(renderer->getDeviceContext());
	textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, shadowMap->getDepthMapSRV());
	textureShader->render(renderer->getDeviceContext(), orthoMesh2->getIndexCount());

	renderer->setZBuffer(true);

	gui();
	renderer->endScene();
}



void App1::gui()
{
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	ImGui::SetNextWindowSize(ImVec2(450, 300), ImGuiCond_Always);

	// Start ImGui
	ImGui::Begin("Controls");

	// Build UI
	ImGui::Text("FPS: %.2f", timer->getFPS());
	ImGui::Checkbox("Wireframe mode", &wireframeToggle);

	ImGui::Text("Brick Cube");
	ImGui::SliderFloat("Translation Speed 1", &cubeTranslationSpeed, 1.0f, 120.0f);

	ImGui::Text("Glass Sphere");
	ImGui::SliderFloat("Translation Speed 2", &sphereTranslationSpeed, 1.0f, 120.0f);

	ImGui::Text("Light Maniuplation");

	ImGui::Checkbox("Use Perspective Projection", &usePerspectiveProjection);

	ImGui::SliderFloat3("Position", (float*)&lightPosition, -50.0f, 50.0f);
	light->setPosition(lightPosition.x, lightPosition.y, lightPosition.z);

	ImGui::SliderFloat3("Direction", (float*)&lightDirection, -1.0f, 1.0f);
	light->setDirection(lightDirection.x, lightDirection.y, lightDirection.z);

	ImGui::SliderFloat("Ambient Intensity", &lightAmbientIntensity, 0.0f, 1.0f);
	light->setAmbientColour(lightAmbientIntensity, lightAmbientIntensity, lightAmbientIntensity, 1.0f);

	ImGui::SliderFloat("Diffuse Intensity", &lightDiffuseIntensity, 0.0f, 1.0f);
	light->setDiffuseColour(lightDiffuseIntensity, lightDiffuseIntensity, lightDiffuseIntensity, 1.0f);

	ImGui::SliderFloat4("Specular Colour", (float*)&lightSpecularColour, 0.0f, 1.0f);
	light->setSpecularColour(lightSpecularColour.x, lightSpecularColour.y, lightSpecularColour.z, 1.0f);

	ImGui::SliderFloat("Specular Power", &lightSpecularPower, 0.0f, 1.0f);
	light->setSpecularPower(lightSpecularPower);

	// End ImGui
	ImGui::End();

	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

