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

	// Building parts
	std::vector<buildingElement> building;
	Plane buildingPlane;
	float buildingPosition[3] = { 0,0,0 };
	float buildingSize[3] = { 1,1,1 };
	int buildingDimension[3] = { 10,7,18 };
	Shader buildingShader;
	unsigned int buildingWallTexture;
	unsigned int buildingFloorTexture;	

	// Painting part
	Grid gridPainting;
	int gridPaintingSize = 250;
	glm::vec3 gridPaintingScale = glm::vec3(0.009375f, 0.009375f, 0.009375f);
	float paintingSize = sqrt(2 * (gridPaintingSize * gridPaintingSize)) / gridPaintingScale[0];
	std::vector<glm::vec3> paintingSlotPosition;
	float paintingYPos = 5.0f;

	// Painting 1 attributs
	Shader painting1Shader;
	float painting1Speed = 7.0f;
	float painting1Amount = 10.0f;
	float painting1Height = 0.4f;
	float painting1Color[3] = { 0.62,0.37,0.62 };

	// Painting 2 attributs
	Shader painting2Shader;
	float painting2Speed = 2.905f;
	float painting2Amount = 21.351f;
	float painting2Height = 0.6f;
	float painting2Angle = 0.5f;
	float painting2Color[3] = { 0.62,0.37,0.62 };
	float painting2Time = 20.27f;

	// Painting 3 attributs
	Shader painting3Shader;
	float painting3Center[2] = { (float)(gridPaintingSize / 2), (float)(gridPaintingSize / 2) };
	float painting3Angle = 0.5f;
	float painting3Height = 5.0f;
	float painting3Speed = 2.0f;
	float painting3Color[3] = { 0.62,0.37,0.62 };

	// Painting 4 attributs
	Shader painting4Shader;
	float painting4Height = 0.912f;
	float painting4Color[3] = { 0.62,0.37,0.62 };
	float painting4Amount = 3.142f;

	// Camera attribut
	float cameraPosition[3] = { 2.25f,4,7.5f };

	Skybox skybox;

	bool debugMod = false;
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

	painting1Shader.CompileSource(
		"shaders/ChaosScene/p1.vert",
		"shaders/ChaosScene/painting.frag"
	);
	shaders.push_back(&painting1Shader);

	painting2Shader.CompileSource(
		"shaders/ChaosScene/p2.vert",
		"shaders/ChaosScene/painting.frag"
	);
	shaders.push_back(&painting2Shader);

	painting3Shader.CompileSource(
		"shaders/ChaosScene/p3.vert",
		"shaders/ChaosScene/painting.frag"
	);
	shaders.push_back(&painting3Shader);

	painting4Shader.CompileSource(
		"shaders/ChaosScene/p4.vert",
		"shaders/ChaosScene/painting.frag"
	);
	shaders.push_back(&painting4Shader);

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

			glm::vec4 position = {
				buildingPosition[0] + (x * buildingSize[0]),
				buildingPosition[1],
				buildingPosition[2] + (z * buildingSize[2]),
				1.0f
			};

			element.modelMatrix = glm::mat4(1.0f);
			element.modelMatrix = glm::translate(element.modelMatrix, glm::vec3(position.x, position.y, position.z));
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

			glm::vec4 position = {
				buildingPosition[0] - buildingSize[0],
				buildingPosition[1] + (y * buildingSize[1]) + buildingSize[1],
				buildingPosition[2] + (z * buildingSize[2]),
				1.0f
			};

			element.modelMatrix = glm::mat4(1.0f);
			element.modelMatrix = glm::translate(element.modelMatrix, glm::vec3(position.x, position.y, position.z));
			element.modelMatrix = glm::rotate(element.modelMatrix, glm::radians(90.0f), glm::vec3(0, 1, 0));
			element.modelMatrix = glm::scale(element.modelMatrix, glm::vec3(buildingSize[0], buildingSize[1], buildingSize[2]));
			
			element.worldPosition = element.modelMatrix * position;

			element.sphereRadius = sqrt(buildingSize[0] * buildingSize[0] + buildingSize[1] * buildingSize[1]) * 30;

			building.push_back(element);

			if (z % 5 == 0)
				paintingSlotPosition.push_back(glm::vec3(position.x, paintingYPos, position.z));
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

			glm::vec4 position = {
				buildingPosition[0] + buildingSize[0] * buildingDimension[0],
				buildingPosition[1] + (y* buildingSize[1]) + buildingSize[1],
				buildingPosition[2] + (z* buildingSize[2]),
				1.0f
			};

			element.modelMatrix = glm::mat4(1.0f);
			element.modelMatrix = glm::translate(element.modelMatrix, glm::vec3(position.x, position.y, position.z));
			element.modelMatrix = glm::rotate(element.modelMatrix, glm::radians(90.0f), glm::vec3(0, 1, 0));
			element.modelMatrix = glm::scale(element.modelMatrix, glm::vec3(buildingSize[0], buildingSize[1], buildingSize[2]));

			element.worldPosition = element.modelMatrix * position;

			element.sphereRadius = sqrt(buildingSize[0] * buildingSize[0] + buildingSize[1] * buildingSize[1]) * 30;

			building.push_back(element);

			if (z % 5 == 0)
				paintingSlotPosition.push_back(glm::vec3(position.x, paintingYPos, position.z));
		}
	}

	// Creating the building back walls
	for (int y = 0; y < buildingDimension[1]; y++)
	{
		for (int x = 0; x < buildingDimension[0]; x++)
		{
			buildingElement element;

			element.texture = buildingWallTexture;

			element.shader = &buildingShader;

			element.plane = &buildingPlane;

			glm::vec4 position = {
				buildingPosition[0] + (x* buildingSize[0]),
				buildingPosition[1] + (y* buildingSize[1]) + buildingSize[1],
				buildingPosition[2] - buildingSize[2],
				1.0f
			};

			element.modelMatrix = glm::mat4(1.0f);
			element.modelMatrix = glm::translate(element.modelMatrix, glm::vec3(position.x, position.y, position.z));
			element.modelMatrix = glm::scale(element.modelMatrix, glm::vec3(buildingSize[0], buildingSize[1], buildingSize[2]));

			element.worldPosition = element.modelMatrix * position;

			element.sphereRadius = sqrt(buildingSize[0] * buildingSize[0] + buildingSize[1] * buildingSize[1]) * 30;

			building.push_back(element);
		}
	}

	// Creating the building front walls
	for (int y = 0; y < buildingDimension[1]; y++)
	{
		for (int x = 0; x < buildingDimension[0]; x++)
		{
			buildingElement element;

			element.texture = buildingWallTexture;

			element.shader = &buildingShader;

			element.plane = &buildingPlane;

			glm::vec4 position = {
				buildingPosition[0] + (x* buildingSize[0]),
				buildingPosition[1] + (y* buildingSize[1]) + buildingSize[1],
				buildingPosition[2] + buildingSize[2] * buildingDimension[2],
				1.0f
			};

			element.modelMatrix = glm::mat4(1.0f);
			element.modelMatrix = glm::translate(element.modelMatrix, glm::vec3(position.x, position.y, position.z));
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

	std::cout << paintingSlotPosition.size();
}

void ChaosSceneDrawingProgram::Draw()
{
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

	int paintingPosIndex = 0;

	glEnable(GL_DEPTH_TEST);
	// Painting 1 rendering
	if(CheckFrustum(paintingSlotPosition[paintingPosIndex], paintingSize))
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		painting1Shader.Bind();
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, paintingSlotPosition[paintingPosIndex]);
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0, 0, 1));
		modelMatrix = glm::scale(modelMatrix, gridPaintingScale);

		painting1Shader.SetMat4("model", modelMatrix);
		painting1Shader.SetMat4("projection", projection);
		painting1Shader.SetMat4("view", camera.GetViewMatrix());
		painting1Shader.SetVec3("vertexColor", painting1Color);
		painting1Shader.SetFloat("speed", painting1Speed);
		painting1Shader.SetFloat("amount", painting1Amount);
		painting1Shader.SetFloat("height", painting1Height);
		painting1Shader.SetFloat("timeSinceStart", engine->GetTimeSinceInit());

		gridPainting.Draw();
		
	}

	paintingPosIndex++;

	// Painting 2 rendering
	if (CheckFrustum(paintingSlotPosition[paintingPosIndex], paintingSize))
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		painting2Shader.Bind();
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, paintingSlotPosition[paintingPosIndex]);
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0, 0, 1));
		modelMatrix = glm::scale(modelMatrix, gridPaintingScale);

		painting2Shader.SetMat4("model", modelMatrix);
		painting2Shader.SetMat4("projection", projection);
		painting2Shader.SetMat4("view", camera.GetViewMatrix());
		painting2Shader.SetVec3("vertexColor", painting2Color);
		painting2Shader.SetVec2("center", painting3Center[0], painting3Center[1]);
		painting2Shader.SetFloat("angle", painting2Angle);
		painting2Shader.SetFloat("speed", painting2Speed);
		painting2Shader.SetFloat("amount", painting2Amount);
		painting2Shader.SetFloat("height", painting2Height);
		painting2Shader.SetFloat("timeSinceStart", painting2Time);

		gridPainting.Draw();
	}

	paintingPosIndex++;

	// Painting 3 rendering
	if (CheckFrustum(paintingSlotPosition[paintingPosIndex], paintingSize))
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		painting3Shader.Bind();
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, paintingSlotPosition[paintingPosIndex]);
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0, 0, 1));
		modelMatrix = glm::scale(modelMatrix, gridPaintingScale);

		painting3Shader.SetMat4("model", modelMatrix);
		painting3Shader.SetMat4("projection", projection);
		painting3Shader.SetMat4("view", camera.GetViewMatrix());
		painting3Shader.SetVec3("viewPos", camera.Position);
		painting3Shader.SetVec3("vertexColor", painting3Color);
		
		painting3Shader.SetVec2("center", painting3Center[0], painting3Center[1]);
		painting3Shader.SetFloat("angle", painting3Angle);
		painting3Shader.SetFloat("speed", painting3Speed);
		painting3Shader.SetFloat("height", painting3Height);

		painting3Shader.SetFloat("timeSinceStart", engine->GetTimeSinceInit());

		gridPainting.Draw();
	}

	paintingPosIndex++;

	// Painting 4 rendering
	if (CheckFrustum(paintingSlotPosition[paintingPosIndex], paintingSize))
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		painting4Shader.Bind();
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, paintingSlotPosition[paintingPosIndex]);
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0, 0, 1));
		modelMatrix = glm::scale(modelMatrix, gridPaintingScale);

		painting4Shader.SetMat4("model", modelMatrix);
		painting4Shader.SetMat4("projection", projection);
		painting4Shader.SetMat4("view", camera.GetViewMatrix());
		painting4Shader.SetVec3("viewPos", camera.Position);
		painting4Shader.SetVec3("vertexColor", painting4Color);

		painting4Shader.SetFloat("height", painting4Height);
		painting4Shader.SetFloat("amount", painting4Amount);

		painting4Shader.SetFloat("timeSinceStart", engine->GetTimeSinceInit());

		gridPainting.Draw();
	}

	paintingPosIndex++;
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

