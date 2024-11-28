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

	// Create camera with perspective of the shadow map
	rtTDebugCamera = new Camera();

	// This is your shadow map
	shadowMap = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);

	// Initialze render to texture
	shadowMapDebugTexture = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);

	// Initialize ortho meshes
	orthoMesh = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(), screenWidth, screenHeight);
	shadowMapDebugOrthoMesh = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(),
		shadowMapDebugTexture->getTextureWidth() / 2, shadowMapDebugTexture->getTextureHeight() / 2,
		-shadowmapWidth/ 2.5, shadowmapHeight / 4.9);

	lights[0] = new Light();
	lights[1] = new Light();

	lights[0]->setPosition(0.f, 50.f, -10.f);
	lights[1]->setPosition(0.f, 50.f, 120.f);

	// Configure directional lights
	for (int i = 0; i < 2; ++i)
	{
		lights[i]->setDirection(0.0f, -0.7f, 0.7f);
		lights[i]->setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
		lights[i]->setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
		lights[i]->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);

		// Configure sphere light for debug
		lightDirections[i] = lights[i]->getDirection();
		lightPositions[i] = lights[i]->getPosition();
		lightAmbientIntensities[i] = 0.3f;
		lightDiffuseIntensities[i] = 1.0f;
		lightSpecularColours[i] = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		lightSpecularPowers[i] = 0.0f;
	}
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

	if (orthoMesh != nullptr)
	{
		delete orthoMesh;
	}

	if (shadowMapDebugOrthoMesh != nullptr)
	{
		delete shadowMapDebugOrthoMesh;
	}

	if (shadowMapDebugTexture != nullptr)
	{
		delete shadowMapDebugTexture;
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
		// Wrap around if we reached 360�
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
	// Render ortho mesh with shadow map for debug
	shadowMapDebugger();
	// Render scene
	finalPass();

	return true;
}

void App1::depthPass()
{
	for (int i = 0; i < 2; ++i)
	{
		// Set the render target to be the render to texture.
		shadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

		// Get the world, view, and projection matrices from the camera and d3d objects.
		lights[i]->generateViewMatrix();
		XMMATRIX lightViewMatrix = lights[i]->getViewMatrix();
		XMMATRIX lightProjectionMatrix = lights[i]->getOrthoMatrix();

		if (usePerspectiveProjection)
		{
			lights[i]->generateProjectionMatrix(0.1f, 100.f);
			lightProjectionMatrix = lights[i]->getProjectionMatrix();
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
		translationMatrix = XMMatrixTranslation(lightPositions[i].x, lightPositions[i].y, lightPositions[i].z);
		scaleMatrix = XMMatrixScaling(10.f, 10.f, 10.f);
		worldMatrix = scaleMatrix * translationMatrix;
		sunlightMesh->sendData(renderer->getDeviceContext());
		depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
		depthShader->render(renderer->getDeviceContext(), sunlightMesh->getIndexCount());
	}

	// Set back buffer as render target and reset view port.
	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();
}

void App1::shadowMapDebugger()
{
	// Set the render target to be the mini map render texture and clear it
	shadowMapDebugTexture->setRenderTarget(renderer->getDeviceContext());
	shadowMapDebugTexture->clearRenderTarget(renderer->getDeviceContext(), 0.0f, 0.0f, 1.0f, 1.0f);

	// Set the camera position and rotation
	rtTDebugCamera->setPosition(camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);
	rtTDebugCamera->setRotation(camera->getRotation().x, camera->getRotation().y, camera->getRotation().z);
	rtTDebugCamera->update();

	// Get matrices
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoViewMatrix = rtTDebugCamera->getOrthoViewMatrix();
	XMMATRIX orthoMatrix = renderer->getOrthoMatrix();

	// RENDER THE RENDER TEXTURE SCENE
	// Requires 2D rendering and an ortho mesh.
	renderer->setZBuffer(false);

	orthoMesh->sendData(renderer->getDeviceContext());
	textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, shadowMap->getDepthMapSRV());
	textureShader->render(renderer->getDeviceContext(), orthoMesh->getIndexCount());

	renderer->setZBuffer(true);

	// Reset render target back to default
	renderer->setBackBufferRenderTarget();
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

	for (int i = 0; i < 2; ++i)
	{
		XMMATRIX lightProjectionMatrix = lights[i]->getOrthoMatrix();

		if (usePerspectiveProjection)
		{
			lights[i]->generateProjectionMatrix(0.1f, 100.f);
			lightProjectionMatrix = lights[i]->getProjectionMatrix();
		}

		worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);

		// Render floor
		planeMesh->sendData(renderer->getDeviceContext());
		shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix,
			textureMgr->getTexture(L"wood"), shadowMap->getDepthMapSRV(), lights, lightProjectionMatrix);
		shadowShader->render(renderer->getDeviceContext(), planeMesh->getIndexCount());

		// Render model
		worldMatrix = renderer->getWorldMatrix();
		XMMATRIX translationMatrix = XMMatrixTranslation(0.f, 4.f, 20.f);
		XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
		XMMATRIX rotationMatrix = XMMatrixRotationY(XMConvertToRadians(teapotRotation));
		worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		model->sendData(renderer->getDeviceContext());
		shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, 
			textureMgr->getTexture(L"ceramic"), shadowMap->getDepthMapSRV(), lights, lightProjectionMatrix);
		shadowShader->render(renderer->getDeviceContext(), model->getIndexCount());

		// Render cube and sphere
		worldMatrix = renderer->getWorldMatrix();
		translationMatrix = XMMatrixTranslation(-3.f, 1.f, 5.f + cubePosition);
		scaleMatrix = XMMatrixScaling(5.f, 5.f, 5.f);
		worldMatrix = translationMatrix * scaleMatrix;
		cubeMesh->sendData(renderer->getDeviceContext());
		shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, 
			textureMgr->getTexture(L"brick"), shadowMap->getDepthMapSRV(), lights, lightProjectionMatrix);
		shadowShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());

		worldMatrix = renderer->getWorldMatrix();
		translationMatrix = XMMatrixTranslation(20.f, 4.f, 15.f + spherePosition);
		scaleMatrix = XMMatrixScaling(4.f, 4.f, 4.f);
		rotationMatrix = XMMatrixRotationX(XMConvertToRadians(sphereRotation));
		worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		sphereMesh->sendData(renderer->getDeviceContext());
		shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, 
			textureMgr->getTexture(L"glass"), shadowMap->getDepthMapSRV(), lights, lightProjectionMatrix);
		shadowShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());

		// Render sun light sphere
		worldMatrix = renderer->getWorldMatrix();
		translationMatrix = XMMatrixTranslation(lightPositions[i].x, lightPositions[i].y, lightPositions[i].z);
		scaleMatrix = XMMatrixScaling(10.f, 10.f, 10.f);
		worldMatrix = scaleMatrix * translationMatrix;
		sunlightMesh->sendData(renderer->getDeviceContext());
		shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, 
			textureMgr->getTexture(L"sun"), shadowMap->getDepthMapSRV(), lights, lightProjectionMatrix);
		shadowShader->render(renderer->getDeviceContext(), sunlightMesh->getIndexCount());
	}

	// Render the shadow map ortho mesh
	renderer->setZBuffer(false);

	// Get ortho matrices
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();
	XMMATRIX orthoMatrix = renderer->getOrthoMatrix();

	shadowMapDebugOrthoMesh->sendData(renderer->getDeviceContext());
	textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, shadowMapDebugTexture->getShaderResourceView());
	textureShader->render(renderer->getDeviceContext(), shadowMapDebugOrthoMesh->getIndexCount());

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
	ImGui::SetNextWindowPos(ImVec2(shadowMapDebugTexture->getTextureWidth() / 1.7, shadowMapDebugTexture->getTextureHeight() / 25), ImGuiCond_Always);

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

	ImGui::SliderFloat3("Position Light 1", (float*)&lightPositions[0], -100.0f, 200.0f);
	lights[0]->setPosition(lightPositions[0].x, lightPositions[0].y, lightPositions[0].z);

	ImGui::SliderFloat3("Position Light 2", (float*)&lightPositions[1], -100.0f, 200.0f);
	lights[1]->setPosition(lightPositions[1].x, lightPositions[1].y, lightPositions[1].z);

	ImGui::SliderFloat3("Direction Light 1", (float*)&lightDirections[0], -1.0f, 1.0f);
	lights[0]->setDirection(lightDirections[0].x, lightDirections[0].y, lightDirections[0].z);
	
	ImGui::SliderFloat3("Direction Light 2", (float*)&lightDirections[1], -1.0f, 1.0f);
	lights[1]->setDirection(lightDirections[1].x, lightDirections[1].y, lightDirections[1].z);

	ImGui::SliderFloat("Ambient Intensity Light 1", &lightAmbientIntensities[0], 0.0f, 1.0f);
	lights[0]->setAmbientColour(lightAmbientIntensities[0], lightAmbientIntensities[0], lightAmbientIntensities[0], 1.0f);
	
	ImGui::SliderFloat("Ambient Intensity Light 2", &lightAmbientIntensities[1], 0.0f, 1.0f);
	lights[1]->setAmbientColour(lightAmbientIntensities[1], lightAmbientIntensities[1], lightAmbientIntensities[1], 1.0f);

	ImGui::SliderFloat("Diffuse Intensity Light 1", &lightDiffuseIntensities[0], 0.0f, 1.0f);
	lights[0]->setDiffuseColour(lightDiffuseIntensities[0], lightDiffuseIntensities[0], lightDiffuseIntensities[0], 1.0f);
	
	ImGui::SliderFloat("Diffuse Intensity Light 2", &lightDiffuseIntensities[1], 0.0f, 1.0f);
	lights[1]->setDiffuseColour(lightDiffuseIntensities[1], lightDiffuseIntensities[1], lightDiffuseIntensities[1], 1.0f);

	ImGui::SliderFloat4("Specular Colour Light 1", (float*) & lightSpecularColours[0], 0.0f, 1.0f);
	lights[0]->setSpecularColour(lightSpecularColours[0].x, lightSpecularColours[0].y, lightSpecularColours[0].z, 1.0f);
	
	ImGui::SliderFloat4("Specular Colour Light 2", (float*) & lightSpecularColours[1], 0.0f, 1.0f);
	lights[1]->setSpecularColour(lightSpecularColours[1].x, lightSpecularColours[1].y, lightSpecularColours[1].z, 1.0f);

	ImGui::SliderFloat("Specular Power Light 1", &lightSpecularPowers[0], 0.0f, 1.0f);
	lights[0]->setSpecularPower(lightSpecularPowers[0]);
	
	ImGui::SliderFloat("Specular Power Light 2", &lightSpecularPowers[1], 0.0f, 1.0f);
	lights[1]->setSpecularPower(lightSpecularPowers[1]);

	// End ImGui
	ImGui::End();

	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

