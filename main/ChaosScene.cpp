#include <map>
#include <vector>

#include <engine.h>
#include <graphics.h>
#include <camera.h>
#include <model.h>
#include <geometry.h>

#include <Remotery.h>
#include "file_utility.h"

#include <json.hpp>
using json = nlohmann::json;
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>

#include "imgui.h"
#include <iostream>

struct buildingElement {
	Plane* plane;
	glm::mat4 modelMatrix;
	glm::vec4 worldPosition;
	unsigned int texture;
	Shader* shader;
	float sphereRadius;
};

struct paintingElement {
	Grid* grid;
	glm::mat4 modelMatrix;
	unsigned int texture;
	Shader* shader;
	float topRight[3];
	float bottomLeft[3];
};

struct frustum {
	glm::vec4 plansNormals[6];
};

class ChaosSceneDrawingProgram : public DrawingProgram
{
public:
	void Init() override;
	void Draw() override;
	void Destroy() override;
	void UpdateUi() override;
	void ProcessInput();
	void BuildFrustum(Camera& camera);
	bool CheckFrustum(glm::vec3 position, float size);
private:
	glm::mat4 projection = {};
	float far = 10000.0f;
	float near = 0.1f;
	float fov = 45.0f;
	frustum mainCameraFrustum;

	std::vector<buildingElement> building;
	std::vector<paintingElement> painting;

	//Grid buildingPlane;
	Plane buildingPlane;
	float buildingPosition[3] = { 0,0,0 };
	float buildingSize[3] = { 1,1,1 };
	int buildingDimension[3] = { 50,5,20 };
	Shader buildingShader;
	unsigned int buildingWallTexture;
	unsigned int buildingFloorTexture;

	Skybox skybox;

	Grid gridPainting;
	int gridPaintingSize = 25;

	// left attributs
	Shader leftShader;
	float leftSpeed = 7.0f;
	float leftAmount = 10.0f;
	float leftHeight = 0.4f;
	float leftPosition[3] = { (float)(-gridPaintingSize / 2),0,(float)(-gridPaintingSize / 2) };
	float leftScale[3] = { 1,1,1 };
	float leftColor[3] = { 0.62,0.37,0.62 };

	// Walls attributs
	Shader wallShader;
	float wallSpeed = 2.905f;
	float wallAmount = 21.351f;
	float wallHeight = 0.6f;
	float wallColor[3] = { 0.62,0.37,0.62 };
	float wallPositionYOffset = 2;

	// Top attributs
	Shader topShader;
	float topCenter[2] = { (float)(gridPaintingSize / 2), (float)(gridPaintingSize / 2) };
	float topAngle = 0.5f;
	float topHeight = 5.0f;
	float topSpeed = 2.0f;
	float topColor[3] = { 0.62,0.37,0.62 };
	float topPositionYOffset = 5;

	// Camera attribut
	float cameraPosition[3] = { 0,(float)(gridPaintingSize / 2),(float)-(gridPaintingSize / 2) };

	bool debugMod = true;
};

void ChaosSceneDrawingProgram::Init()
{
	programName = "Chaos scene";
	Engine* engine = Engine::GetPtr();
	auto& config = engine->GetConfiguration();
	Camera& camera = engine->GetCamera();

	gridPainting.Init(gridPaintingSize);
	
	buildingWallTexture = gliCreateTexture("data/sprites/wall.dds");
	buildingFloorTexture = gliCreateTexture("data/sprites/floor.dds");

	glBindTexture(GL_TEXTURE_2D, buildingWallTexture);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, buildingFloorTexture);
	glGenerateMipmap(GL_TEXTURE_2D);

	buildingShader.CompileSource(
		"shaders/ChaosScene/building.vert",
		"shaders/ChaosScene/building.frag");
	shaders.push_back(&buildingShader);

	leftShader.CompileSource(
		"shaders/ChaosScene/floor.vert",
		"shaders/ChaosScene/floor.frag"
	);
	shaders.push_back(&leftShader);

	wallShader.CompileSource(
		"shaders/ChaosScene/wall.vert",
		"shaders/ChaosScene/wall.frag"
	);
	shaders.push_back(&wallShader);

	topShader.CompileSource(
		"shaders/ChaosScene/top.vert",
		"shaders/ChaosScene/top.frag"
	);
	shaders.push_back(&topShader);

	camera.Position.x = cameraPosition[0];
	camera.Position.y = cameraPosition[1];
	camera.Position.z = cameraPosition[2];

	buildingPlane.Init();

	// Create the building floor
	for (int x = 0; x < buildingDimension[0]; x++)
	{
		for (int z = 0; z < buildingDimension[2]; z++)
		{
			buildingElement element;

			element.texture = buildingFloorTexture;

			element.shader = &buildingShader;

			element.plane = &buildingPlane;

			float positionX = buildingPosition[0] + (x * buildingSize[0]);
			float positionY = buildingPosition[1];
			float positionZ = buildingPosition[2] + (z * buildingSize[2]);

			glm::vec4 position = {
				positionX,
				positionY,
				positionZ,
				1.0f
			};

			element.modelMatrix = glm::mat4(1.0f);
			element.modelMatrix = glm::translate(element.modelMatrix, glm::vec3(positionX, positionY, positionZ));
			element.modelMatrix = glm::rotate(element.modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));
			element.modelMatrix = glm::scale(element.modelMatrix, glm::vec3(buildingSize[0], buildingSize[1], buildingSize[2]));

			element.worldPosition = element.modelMatrix * position;

			element.sphereRadius = sqrt(buildingSize[0] * buildingSize[0] + buildingSize[1] * buildingSize[1]) * 30;

			building.push_back(element);
		}
	}

	// Creating the building right walls
	for (int y = 0; y < buildingDimension[1]; y++)
	{
		for (int z = 0; z < buildingDimension[2]; z++)
		{
			buildingElement element;

			element.texture = buildingWallTexture;

			element.shader = &buildingShader;

			element.plane = &buildingPlane;

			float positionX = buildingPosition[0] - buildingSize[0];
			float positionY = buildingPosition[1];
			float positionZ = buildingPosition[2] + (z * buildingSize[2]);

			glm::vec4 position = {
				positionX,
				positionY,
				positionZ,
				1.0f
			};

			element.modelMatrix = glm::mat4(1.0f);
			element.modelMatrix = glm::translate(element.modelMatrix, glm::vec3(buildingPosition[0] - buildingSize[0], buildingPosition[1] + (y * buildingSize[1]) + buildingSize[1], buildingPosition[2] + (z * buildingSize[2])));
			element.modelMatrix = glm::rotate(element.modelMatrix, glm::radians(90.0f), glm::vec3(0, 1, 0));
			element.modelMatrix = glm::scale(element.modelMatrix, glm::vec3(buildingSize[0], buildingSize[1], buildingSize[2]));
			
			element.worldPosition = element.modelMatrix * position;

			element.sphereRadius = sqrt(buildingSize[0] * buildingSize[0] + buildingSize[1] * buildingSize[1]) * 30;

			building.push_back(element);
		}
	}

	// Creating the building left walls
	for (int y = 0; y < buildingDimension[1]; y++)
	{
		for (int z = 0; z < buildingDimension[2]; z++)
		{
			buildingElement element;

			element.texture = buildingWallTexture;

			element.shader = &buildingShader;

			element.plane = &buildingPlane;

			float positionX = buildingPosition[0] + buildingSize[0] * buildingDimension[0];
			float positionY = buildingPosition[1] + (y * buildingSize[1]) + buildingSize[1];
			float positionZ = buildingPosition[2] + (z * buildingSize[2]);

			glm::vec4 position = {
				positionX,
				positionY,
				positionZ,
				1.0f
			};

			element.modelMatrix = glm::mat4(1.0f);
			element.modelMatrix = glm::translate(element.modelMatrix, glm::vec3(buildingPosition[0] + buildingSize[0] * buildingDimension[0], buildingPosition[1] + (y * buildingSize[1]) + buildingSize[1], buildingPosition[2] + (z * buildingSize[2])));
			element.modelMatrix = glm::rotate(element.modelMatrix, glm::radians(90.0f), glm::vec3(0, 1, 0));
			element.modelMatrix = glm::scale(element.modelMatrix, glm::vec3(buildingSize[0], buildingSize[1], buildingSize[2]));

			element.worldPosition = element.modelMatrix * position;

			element.sphereRadius = sqrt(buildingSize[0] * buildingSize[0] + buildingSize[1] * buildingSize[1]) * 30;

			building.push_back(element);
		}
	}

	// Creating the building bottom walls
	for (int y = 0; y < buildingDimension[1]; y++)
	{
		for (int x = 0; x < buildingDimension[0]; x++)
		{
			buildingElement element;

			element.texture = buildingWallTexture;

			element.shader = &buildingShader;

			element.plane = &buildingPlane;

			float positionX = buildingPosition[0] + (x * buildingSize[0]);
			float positionY = buildingPosition[1] + (y * buildingSize[1]) + buildingSize[1];
			float positionZ = buildingPosition[2] - buildingSize[2];

			glm::vec4 position = {
				positionX,
				positionY,
				positionZ,
				1.0f
			};

			element.modelMatrix = glm::mat4(1.0f);
			element.modelMatrix = glm::translate(element.modelMatrix, glm::vec3(buildingPosition[0] + (x * buildingSize[0]), buildingPosition[1] + (y * buildingSize[1]) + buildingSize[1], buildingPosition[2] - buildingSize[2]));
			element.modelMatrix = glm::scale(element.modelMatrix, glm::vec3(buildingSize[0], buildingSize[1], buildingSize[2]));

			element.worldPosition = element.modelMatrix * position;

			element.sphereRadius = sqrt(buildingSize[0] * buildingSize[0] + buildingSize[1] * buildingSize[1]) * 30;

			building.push_back(element);
		}
	}

	// Creating the building top walls
	for (int y = 0; y < buildingDimension[1]; y++)
	{
		for (int x = 0; x < buildingDimension[0]; x++)
		{
			buildingElement element;

			element.texture = buildingWallTexture;

			element.shader = &buildingShader;

			element.plane = &buildingPlane;

			float positionX = buildingPosition[0] + (x * buildingSize[0]);
			float positionY = buildingPosition[1] + (y * buildingSize[1]) + buildingSize[1];
			float positionZ = buildingPosition[2] + buildingSize[2] * buildingDimension[2];

			glm::vec4 position = {
				positionX,
				positionY,
				positionZ,
				1.0f
			};

			element.modelMatrix = glm::mat4(1.0f);
			element.modelMatrix = glm::translate(element.modelMatrix, glm::vec3(buildingPosition[0] + (x * buildingSize[0]), buildingPosition[1] + (y * buildingSize[1]) + buildingSize[1], buildingPosition[2] + buildingSize[2] * buildingDimension[2]));
			element.modelMatrix = glm::scale(element.modelMatrix, glm::vec3(buildingSize[0], buildingSize[1], buildingSize[2]));

			element.worldPosition = element.modelMatrix * position;
			
			element.sphereRadius = sqrt(buildingSize[0] * buildingSize[0] + buildingSize[1] * buildingSize[1]) * 30;

			building.push_back(element);
		}
	}

	std::vector<std::string> faces =
	{
		"data/skybox/fluffballday/FluffballDayLeft.hdr",
		"data/skybox/fluffballday/FluffballDayRight.hdr",
		"data/skybox/fluffballday/FluffballDayTop.hdr",
		"data/skybox/fluffballday/FluffballDayBottom.hdr",
		"data/skybox/fluffballday/FluffballDayFront.hdr",
		"data/skybox/fluffballday/FluffballDayBack.hdr"
	};

	skybox.Init(faces);
}

void ChaosSceneDrawingProgram::Draw()
{
	/*rmt_ScopedOpenGLSample(DrawWater);
	rmt_ScopedCPUSample(DrawWaterCPU, 0);*/
	Engine* engine = Engine::GetPtr();
	auto& config = engine->GetConfiguration();
	Camera& camera = engine->GetCamera();

	projection = glm::perspective(glm::radians(fov), (float)config.screenWidth / (float)config.screenHeight, near, far);

	BuildFrustum(camera);

	skybox.SetViewMatrix(camera.GetViewMatrix());
	skybox.SetProjectionMatrix(projection);
	skybox.Draw();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	ProcessInput();

	// building rendering
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		for (int i = 0; i < building.size(); i++)
		{
			if (!CheckFrustum(building[i].worldPosition, building[i].sphereRadius))
				continue;

			building[i].shader->Bind();

			building[i].shader->SetMat4("projection", projection);
			building[i].shader->SetMat4("view", camera.GetViewMatrix());
			building[i].shader->SetInt("activeTexture", 0);
			building[i].shader->SetMat4("model", building[i].modelMatrix);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, building[i].texture);

			building[i].plane->Draw();
		}
	}

	glEnable(GL_DEPTH_TEST);
	// left rendering
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		leftShader.Bind();
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(leftPosition[0] + gridPaintingSize, leftPosition[1] + gridPaintingSize, leftPosition[2] - (gridPaintingSize * 1.25)));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(45.0f), glm::vec3(0, 0, 1));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(leftScale[0], leftScale[1], leftScale[2]));

		leftShader.SetMat4("model", modelMatrix);
		leftShader.SetMat4("projection", projection);
		leftShader.SetMat4("view", camera.GetViewMatrix());
		leftShader.SetVec3("vertexColor", leftColor);
		leftShader.SetFloat("speed", leftSpeed);
		leftShader.SetFloat("amount", leftAmount);
		leftShader.SetFloat("height", leftHeight);
		leftShader.SetFloat("timeSinceStart", engine->GetTimeSinceInit());

		gridPainting.Draw();
	}

	// right rendering
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		wallShader.Bind();
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(leftPosition[0] - gridPaintingSize * 0.72, leftPosition[1] + gridPaintingSize, leftPosition[2] - (gridPaintingSize * 0.57)));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(-45.0f), glm::vec3(0, 0, 1));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(leftScale[0], leftScale[1], leftScale[2]));

		wallShader.SetMat4("model", modelMatrix);
		wallShader.SetMat4("projection", projection);
		wallShader.SetMat4("view", camera.GetViewMatrix());
		wallShader.SetVec3("vertexColor", wallColor);
		topShader.SetVec2("center", topCenter[0], topCenter[1]);
		wallShader.SetFloat("speed", wallSpeed);
		wallShader.SetFloat("amount", wallAmount);
		wallShader.SetFloat("height", wallHeight);
		wallShader.SetFloat("timeSinceStart", engine->GetTimeSinceInit());

		gridPainting.Draw();
	}

	// Top rendering
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		topShader.Bind();
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(leftPosition[0], leftPosition[1] + gridPaintingSize, leftPosition[2] - (gridPaintingSize * 1.25)));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(leftScale[0], leftScale[1], leftScale[2]));

		topShader.SetMat4("model", modelMatrix);
		topShader.SetMat4("projection", projection);
		topShader.SetMat4("view", camera.GetViewMatrix());
		topShader.SetVec3("viewPos", camera.Position);
		topShader.SetVec3("vertexColor", topColor);
		
		topShader.SetVec2("center", topCenter[0], topCenter[1]);
		topShader.SetFloat("angle", topAngle);
		topShader.SetFloat("speed", topSpeed);
		topShader.SetFloat("height", topHeight);

		topShader.SetFloat("timeSinceStart", engine->GetTimeSinceInit());

		gridPainting.Draw();
	}
}

void ChaosSceneDrawingProgram::BuildFrustum(Camera& camera)
{
	glm::mat4 cv = camera.GetViewMatrix();
	glm::mat4 vp;

	vp[0][0] = cv[0][0] * projection[0][0] + cv[0][1] * projection[1][0] + cv[0][2] * projection[2][0] + cv[0][3] * projection[3][0];
	vp[0][1] = cv[0][0] * projection[0][1] + cv[0][1] * projection[1][1] + cv[0][2] * projection[2][1] + cv[0][3] * projection[3][1];
	vp[0][2] = cv[0][0] * projection[0][2] + cv[0][1] * projection[1][2] + cv[0][2] * projection[2][2] + cv[0][3] * projection[3][2];
	vp[0][3] = cv[0][0] * projection[0][3] + cv[0][1] * projection[1][3] + cv[0][2] * projection[2][3] + cv[0][3] * projection[3][3];
										  
	vp[1][0] = cv[1][0] * projection[0][0] + cv[1][1] * projection[1][0] + cv[1][2] * projection[2][0] + cv[1][3] * projection[3][0];
	vp[1][1] = cv[1][0] * projection[0][1] + cv[1][1] * projection[1][1] + cv[1][2] * projection[2][1] + cv[1][3] * projection[3][1];
	vp[1][2] = cv[1][0] * projection[0][2] + cv[1][1] * projection[1][2] + cv[1][2] * projection[2][2] + cv[1][3] * projection[3][2];
	vp[1][3] = cv[1][0] * projection[0][3] + cv[1][1] * projection[1][3] + cv[1][2] * projection[2][3] + cv[1][3] * projection[3][3];
										  
	vp[2][0] = cv[2][0] * projection[0][0] + cv[2][1] * projection[1][0] + cv[2][2] * projection[2][0] + cv[2][3] * projection[3][0];
	vp[2][1] = cv[2][0] * projection[0][1] + cv[2][1] * projection[1][1] + cv[2][2] * projection[2][1] + cv[2][3] * projection[3][1];
	vp[2][2] = cv[2][0] * projection[0][2] + cv[2][1] * projection[1][2] + cv[2][2] * projection[2][2] + cv[2][3] * projection[3][2];
	vp[2][3] = cv[2][0] * projection[0][3] + cv[2][1] * projection[1][3] + cv[2][2] * projection[2][3] + cv[2][3] * projection[3][3];
										  
	vp[3][0] = cv[3][0] * projection[0][0] + cv[3][1] * projection[1][0] + cv[3][2] * projection[2][0] + cv[3][3] * projection[3][0];
	vp[3][1] = cv[3][0] * projection[0][1] + cv[3][1] * projection[1][1] + cv[3][2] * projection[2][1] + cv[3][3] * projection[3][1];
	vp[3][2] = cv[3][0] * projection[0][2] + cv[3][1] * projection[1][2] + cv[3][2] * projection[2][2] + cv[3][3] * projection[3][2];
	vp[3][3] = cv[3][0] * projection[0][3] + cv[3][1] * projection[1][3] + cv[3][2] * projection[2][3] + cv[3][3] * projection[3][3];

	// Near plane
	mainCameraFrustum.plansNormals[0].x = vp[0][3] + vp[0][2];
	mainCameraFrustum.plansNormals[0].y = vp[1][3] + vp[1][2];
	mainCameraFrustum.plansNormals[0].z = vp[2][3] + vp[2][2];
	mainCameraFrustum.plansNormals[0].w = vp[3][3] + vp[3][2];

	// Normalize near plane
	mainCameraFrustum.plansNormals[0] = 
		mainCameraFrustum.plansNormals[0] / 
		sqrt(pow(mainCameraFrustum.plansNormals[0].x, 2) + 
			pow(mainCameraFrustum.plansNormals[0].y, 2) + 
			pow(mainCameraFrustum.plansNormals[0].z, 2));

	// Far plane
	mainCameraFrustum.plansNormals[1].x = vp[0][3] - vp[0][2];
	mainCameraFrustum.plansNormals[1].y = vp[1][3] - vp[1][2];
	mainCameraFrustum.plansNormals[1].z = vp[2][3] - vp[2][2];
	mainCameraFrustum.plansNormals[1].w = vp[3][3] - vp[3][2];

	// Normalize far plane
	mainCameraFrustum.plansNormals[1] =
		mainCameraFrustum.plansNormals[1] /
		sqrt(pow(mainCameraFrustum.plansNormals[1].x, 2) +
			pow(mainCameraFrustum.plansNormals[1].y, 2) +
			pow(mainCameraFrustum.plansNormals[1].z, 2));

	// Bottom plane
	mainCameraFrustum.plansNormals[2].x = vp[0][3] + vp[0][1];
	mainCameraFrustum.plansNormals[2].y = vp[1][3] + vp[1][1];
	mainCameraFrustum.plansNormals[2].z = vp[2][3] + vp[2][1];
	mainCameraFrustum.plansNormals[2].w = vp[3][3] + vp[3][1];

	// Normalize bottom plane
	mainCameraFrustum.plansNormals[2] =
		mainCameraFrustum.plansNormals[2] /
		sqrt(pow(mainCameraFrustum.plansNormals[2].x, 2) +
			pow(mainCameraFrustum.plansNormals[2].y, 2) +
			pow(mainCameraFrustum.plansNormals[2].z, 2));

	// Top plane
	mainCameraFrustum.plansNormals[3].x = vp[0][3] - vp[0][1];
	mainCameraFrustum.plansNormals[3].y = vp[1][3] - vp[1][1];
	mainCameraFrustum.plansNormals[3].z = vp[2][3] - vp[2][1];
	mainCameraFrustum.plansNormals[3].w = vp[3][3] - vp[3][1];

	// Normalize top plane
	mainCameraFrustum.plansNormals[3] =
		mainCameraFrustum.plansNormals[3] /
		sqrt(pow(mainCameraFrustum.plansNormals[3].x, 2) +
			pow(mainCameraFrustum.plansNormals[3].y, 2) +
			pow(mainCameraFrustum.plansNormals[3].z, 2));

	// Left plane
	mainCameraFrustum.plansNormals[4].x = vp[0][3] + vp[0][0];
	mainCameraFrustum.plansNormals[4].y = vp[1][3] + vp[1][0];
	mainCameraFrustum.plansNormals[4].z = vp[2][3] + vp[2][0];
	mainCameraFrustum.plansNormals[4].w = vp[3][3] + vp[3][0];

	// Normalize left plane
	mainCameraFrustum.plansNormals[4] =
		mainCameraFrustum.plansNormals[4] /
		sqrt(pow(mainCameraFrustum.plansNormals[4].x, 2) +
			pow(mainCameraFrustum.plansNormals[4].y, 2) +
			pow(mainCameraFrustum.plansNormals[4].z, 2));

	// Right plane
	mainCameraFrustum.plansNormals[5].x = vp[0][3] - vp[0][0];
	mainCameraFrustum.plansNormals[5].y = vp[1][3] - vp[1][0];
	mainCameraFrustum.plansNormals[5].z = vp[2][3] - vp[2][0];
	mainCameraFrustum.plansNormals[5].w = vp[3][3] - vp[3][0];

	// Normalize right plane
	mainCameraFrustum.plansNormals[5] =
		mainCameraFrustum.plansNormals[5] /
		sqrt(pow(mainCameraFrustum.plansNormals[5].x, 2) +
			pow(mainCameraFrustum.plansNormals[5].y, 2) +
			pow(mainCameraFrustum.plansNormals[5].z, 2));
}

bool ChaosSceneDrawingProgram::CheckFrustum(glm::vec3 position, float size)
{
	for (int i = 0; i < mainCameraFrustum.plansNormals->length(); i++)
	{
		if (mainCameraFrustum.plansNormals[i].x * position.x +
			mainCameraFrustum.plansNormals[i].y * position.y +
			mainCameraFrustum.plansNormals[i].z * position.z +
			mainCameraFrustum.plansNormals[i].w <= -size)
		{
			return debugMod;
		}
	}
	return !debugMod;
}

void ChaosSceneDrawingProgram::Destroy()
{
	
}

void ChaosSceneDrawingProgram::UpdateUi()
{
	ImGui::Separator();
	ImGui::Checkbox("Debug mod", &debugMod);
	ImGui::SliderFloat("Camera far", &far, 15.0f, 1000.0f);
	ImGui::SliderFloat("Camera near", &near, 0.0f, 15.0f);
	ImGui::SliderFloat("Camera fov", &fov, 0.0f, 120.0f);
}

void ChaosSceneDrawingProgram::ProcessInput()
{
	Engine* engine = Engine::GetPtr();
	auto& inputManager = engine->GetInputManager();
	auto& camera = engine->GetCamera();
	float dt = engine->GetDeltaTime();
	float cameraSpeed = 100000.0f;

#ifdef USE_SDL2
	if (inputManager.GetButton(SDLK_w))
	{
		camera.ProcessKeyboard(FORWARD, engine->GetDeltaTime());
	}
	if (inputManager.GetButton(SDLK_s))
	{
		camera.ProcessKeyboard(BACKWARD, engine->GetDeltaTime());
	}
	if (inputManager.GetButton(SDLK_a))
	{
		camera.ProcessKeyboard(LEFT, engine->GetDeltaTime());
	}
	if (inputManager.GetButton(SDLK_d))
	{
		camera.ProcessKeyboard(RIGHT, engine->GetDeltaTime());
	}
	if (inputManager.GetButton(SDLK_RSHIFT))
	{
		debugMod = !debugMod;
	}
#endif

	auto mousePos = inputManager.GetMousePosition();

	camera.ProcessMouseMovement(mousePos.x, mousePos.y, true);

	camera.ProcessMouseScroll(inputManager.GetMouseWheelDelta());
}

int main(int argc, char** argv)
{
	Engine engine;
	auto& config = engine.GetConfiguration();
	config.screenWidth = 1920;
	config.screenHeight = 1017;
	config.windowName = "Chaos scene";
	config.bgColor.r = 0;
	config.bgColor.g = 0;
	config.bgColor.b = 0;
	engine.AddDrawingProgram(new ChaosSceneDrawingProgram());

	engine.Init();
	engine.GameLoop();

	return EXIT_SUCCESS;
}

