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

class SceneDrawingProgram;

class WaterSceneDrawingProgram : public DrawingProgram
{
public:
	void Init() override;
	void Draw() override;
	void Destroy() override;
	void UpdateUi() override;

	void InitWaterGrid();
	void UpdateGridHeights();
	void PerturbateGrid();
private:
	Camera underWaterCamera;
	Camera* sceneCamera = nullptr;
	SceneDrawingProgram* scene = nullptr;

	Shader basicShader; //just to render to the stencil buffer

	int waterSize = 250;
	float waterHeightRefreshOffset = 0.064f;
	float drawTime = 0.0f;

	std::vector<glm::vec3> currentWaterGrid;
	std::vector<glm::vec3> previousWaterGrid;
	std::vector<short> waterIndices;
	Grid wGrid;
	GLuint waterVAO;
	GLuint waterVBO;
	GLuint waterEBO;

	float waterSpeed = 1.0f;
	float waterAmount = 10.0f;
	float waterHeight = 1.0f;

	float minPerturbationHeight = 1.0f;
	float maxPerturbationHeight = 4.0f;
	float noPerturbationSince = 0.0f;
	float perturbationCoolDown = 0.01f;

	Shader reflectionModelShader;
	unsigned int reflectionColorBuffer;
	unsigned int reflectionBuffer;
	unsigned int reflectionDepthBuffer;
	Shader frameBufferShaderProgram;

	Shader refractionModelShader;
	unsigned int refractionColorBuffer;
	unsigned int refractionDepthBuffer;
	unsigned int refractionBuffer;
	unsigned int refractionStencilBuffer;

	float refractionStrengh = 0.3f;
	float waterRefractionRatio = 0.5f;
};

class SceneDrawingProgram : public DrawingProgram
{
public:
	void Init() override;
	void Draw() override;
	void Destroy() override;
	void UpdateUi() override;
	//Getters for Water Drawing Program
	glm::mat4& GetProjection() { return projection; }
	std::vector<Model*>& GetModels() { return models; }
	std::vector<glm::vec3>& GetPositions() { return positions; }
	std::vector<glm::vec3>& GetScales() { return scales; }
	std::vector<glm::vec3>& GetRotations() { return rotations; }
	size_t GetModelNmb() { return modelNmb; }
	Skybox& GetSkybox() { return skybox; }
protected:

	void ProcessInput();
	Skybox skybox;
	Shader modelShader;
	glm::mat4 projection = {};
	const char* jsonPath = "scenes/water.json";
	json sceneJson;
	std::map<std::string, Model> modelMap;
	std::vector<Model*> models;
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> scales;
	std::vector<glm::vec3> rotations;

	size_t modelNmb;

	float lastX = 0;
	float lastY = 0;

};


void WaterSceneDrawingProgram::Init()
{
	programName = "Under Water Drawing";
	Engine* engine = Engine::GetPtr();
	auto& config = engine->GetConfiguration();
	auto& drawingPrograms = engine->GetDrawingPrograms();

	scene = dynamic_cast<SceneDrawingProgram*>(drawingPrograms[0]);
	sceneCamera = &engine->GetCamera();
	sceneCamera->Position = glm::vec3(0.0f, 3.0f, 10.0f);
	sceneCamera->MovementSpeed *= 5;
	basicShader.CompileSource(
		"shaders/engine/basic.vert",
		"shaders/engine/basic.frag");
	shaders.push_back(&basicShader);

	srand(time(NULL));
	//wGrid.Init(waterSize);
	currentWaterGrid.resize(waterSize * waterSize);
	previousWaterGrid.resize(waterSize * waterSize);
	waterIndices.resize(waterSize * waterSize * 6);
	InitWaterGrid();

	glGenVertexArrays(1, &waterVAO);
	glGenBuffers(1, &waterVBO);
	glGenBuffers(1, &waterEBO);
	//bind water quad (need tex coords for the final rendering
	glBindVertexArray(waterVAO);

	glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
	glBufferData(GL_ARRAY_BUFFER, currentWaterGrid.size() * sizeof(glm::vec3), NULL, GL_STREAM_DRAW);
	//glNamedBufferSubData(waterVBO, 0, currentWaterGrid.size() * sizeof(glm::vec3), &currentWaterGrid[0]);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterIndices.size() * sizeof(short), &waterIndices[0], GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// tex coords attribute
	glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_FALSE, 5 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	reflectionModelShader.CompileSource(
		"shaders/WaterScene/reflection.vert",
		"shaders/WaterScene/reflection.frag");
	shaders.push_back(&reflectionModelShader);

	//Generate under water refelction map framebuffer
	glGenFramebuffers(1, &reflectionBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, reflectionBuffer);

	glGenTextures(1, &reflectionColorBuffer);
	glBindTexture(GL_TEXTURE_2D, reflectionColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, config.screenWidth, config.screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionColorBuffer, 0);

	glGenRenderbuffers(1, &reflectionDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, reflectionDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, config.screenWidth, config.screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, reflectionDepthBuffer);
	frameBufferShaderProgram.CompileSource(
		"shaders/WaterScene/blending.vert",
		"shaders/WaterScene/blending.frag"
	);

	//Generate under water refelction map framebuffer
	refractionModelShader.CompileSource(
		"shaders/WaterScene/refraction.vert",
		"shaders/WaterScene/refraction.frag");
	shaders.push_back(&refractionModelShader);
	glGenFramebuffers(1, &refractionBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, refractionBuffer);

	glGenTextures(1, &refractionColorBuffer);
	glBindTexture(GL_TEXTURE_2D, refractionColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, config.screenWidth, config.screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, refractionColorBuffer, 0);

	glGenTextures(1, &refractionDepthBuffer);
	glBindTexture(GL_TEXTURE_2D, refractionDepthBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		config.screenWidth, config.screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindFramebuffer(GL_FRAMEBUFFER, refractionBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, refractionDepthBuffer, 0);

	glGenRenderbuffers(1, &refractionStencilBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, refractionStencilBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX1, config.screenWidth, config.screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, refractionDepthBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WaterSceneDrawingProgram::Draw()
{
	rmt_ScopedOpenGLSample(DrawWater);
	rmt_ScopedCPUSample(DrawWaterCPU, 0);
	Engine* engine = Engine::GetPtr();
	auto& config = engine->GetConfiguration();
	Skybox& skybox = scene->GetSkybox();

	if (noPerturbationSince >= perturbationCoolDown)
	{
		PerturbateGrid();
		noPerturbationSince = 0.0f;
	}
	else
		noPerturbationSince += engine->GetDeltaTime();

	if (drawTime >= waterHeightRefreshOffset)
	{
		UpdateGridHeights();
		drawTime = 0.0f;
	}
	else
		drawTime += engine->GetDeltaTime();

	//std::cout << engine->GetDeltaTime() << "\n";

	glm::mat4 stencilWaterModel = glm::mat4(1.0f);
	//==================REFLECTION==================
	{
		//Invert Camera
		underWaterCamera.Front = glm::reflect(sceneCamera->Front, glm::vec3(0.0f, 1.0f, 0.0f));
		underWaterCamera.Up = glm::reflect(sceneCamera->Up, glm::vec3(0.0f, 1.0f, 0.0f));
		underWaterCamera.Position = sceneCamera->Position;
		underWaterCamera.Position.y = underWaterCamera.Position.y - 2 * abs(underWaterCamera.Position.y - waterHeight);

		//Under Water framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, reflectionBuffer);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Draw scene on top of water in reflection map
		reflectionModelShader.Bind();
		reflectionModelShader.SetFloat("waterHeight", waterHeight);
		reflectionModelShader.SetMat4("projection", scene->GetProjection());
		reflectionModelShader.SetMat4("view", underWaterCamera.GetViewMatrix());
		reflectionModelShader.SetFloat("speed", waterSpeed);
		reflectionModelShader.SetFloat("amount", waterAmount);
		reflectionModelShader.SetFloat("height", waterHeight);
		reflectionModelShader.SetFloat("timeSinceStart", engine->GetTimeSinceInit());
		for (auto i = 0l; i < scene->GetModelNmb(); i++)
		{
			glm::mat4 modelMatrix(1.0f);
			modelMatrix = glm::translate(modelMatrix, scene->GetPositions()[i]);
			modelMatrix = glm::scale(modelMatrix, scene->GetScales()[i]);
			glm::quat quaternion = glm::quat(scene->GetRotations()[i]);
			modelMatrix = glm::mat4_cast(quaternion) * modelMatrix;

			reflectionModelShader.SetMat4("model", modelMatrix);

			scene->GetModels()[i]->Draw(reflectionModelShader);
		}

		const auto underWaterView = underWaterCamera.GetViewMatrix();
		skybox.SetViewMatrix(underWaterView);
		skybox.SetProjectionMatrix(scene->GetProjection());
		skybox.Draw();

	}
	//==================REFRACTION==================
	{

		glBindFramebuffer(GL_FRAMEBUFFER, refractionBuffer);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Draw scene on top of water in reflection map
		refractionModelShader.Bind();
		refractionModelShader.SetFloat("waterHeight", waterHeight);
		refractionModelShader.SetMat4("projection", scene->GetProjection());
		refractionModelShader.SetMat4("view", sceneCamera->GetViewMatrix());
		refractionModelShader.SetFloat("speed", waterSpeed);
		refractionModelShader.SetFloat("amount", waterAmount);
		refractionModelShader.SetFloat("height", waterHeight);
		refractionModelShader.SetFloat("timeSinceStart", engine->GetTimeSinceInit());
		for (auto i = 0l; i < scene->GetModelNmb(); i++)
		{
			glm::mat4 modelMatrix(1.0f);
			modelMatrix = glm::translate(modelMatrix, scene->GetPositions()[i]);
			modelMatrix = glm::scale(modelMatrix, scene->GetScales()[i]);
			glm::quat quaternion = glm::quat(scene->GetRotations()[i]);
			modelMatrix = glm::mat4_cast(quaternion) * modelMatrix;

			refractionModelShader.SetMat4("model", modelMatrix);

			scene->GetModels()[i]->Draw(refractionModelShader);
		}
		skybox.SetViewMatrix(sceneCamera->GetViewMatrix());
		skybox.SetProjectionMatrix(scene->GetProjection());
		skybox.Draw();
	}
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		frameBufferShaderProgram.Bind();
		stencilWaterModel = glm::mat4(1.0f);
		stencilWaterModel = glm::translate(stencilWaterModel, glm::vec3(0, waterHeight, 0));

		stencilWaterModel = glm::scale(stencilWaterModel, glm::vec3(0.5f, 0.5f, 0.5f));

		frameBufferShaderProgram.SetMat4("model", stencilWaterModel);
		frameBufferShaderProgram.SetMat4("projection", scene->GetProjection());
		frameBufferShaderProgram.SetMat4("view", sceneCamera->GetViewMatrix());
		frameBufferShaderProgram.SetVec3("viewPos", sceneCamera->Position);
		frameBufferShaderProgram.SetFloat("refractionStrengh", refractionStrengh);
		frameBufferShaderProgram.SetFloat("indecesOfRefractionRatio", waterRefractionRatio);
		frameBufferShaderProgram.SetFloat("waveStrength", waterSpeed);
		frameBufferShaderProgram.SetFloat("speed", waterSpeed);
		frameBufferShaderProgram.SetFloat("amount", waterAmount);
		frameBufferShaderProgram.SetFloat("height", waterHeight);
		frameBufferShaderProgram.SetFloat("timeSinceStart", engine->GetTimeSinceInit());
		frameBufferShaderProgram.SetInt("reflectionMap", 0);
		frameBufferShaderProgram.SetInt("refractionMap", 1);
		frameBufferShaderProgram.SetInt("depthMap", 3);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, reflectionColorBuffer);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, refractionColorBuffer);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, refractionDepthBuffer);

		glBindVertexArray(waterVAO);
		glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
		glDrawElements(GL_TRIANGLES, (waterIndices.size()), GL_UNSIGNED_SHORT, (void*)0);
		glBindVertexArray(0);

		//wGrid.Draw();
	}
}

void WaterSceneDrawingProgram::Destroy()
{
	currentWaterGrid.clear();
	previousWaterGrid.clear();
	waterIndices.clear();
}

void WaterSceneDrawingProgram::UpdateUi()
{
	ImGui::Separator();
	ImGui::SliderFloat("reflection strengh", &refractionStrengh, -1.0f, 1.0f);
	ImGui::SliderFloat("refraction indeces ratio", &waterRefractionRatio, -1.0f, 1.0f);
	ImGui::SliderFloat("water speed", &waterSpeed, 0.0f, 5.0f);
	ImGui::SliderFloat("water height", &waterHeight, 0.0f, 10.0f);
	ImGui::SliderFloat("water amount", &waterAmount, 0.001f, 0.1f);
}

void WaterSceneDrawingProgram::InitWaterGrid()
{
	for (int x = 0; x < waterSize; x++)
	{
		for (int z = 0; z < waterSize; z++)
		{
			float height = minPerturbationHeight + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (maxPerturbationHeight - minPerturbationHeight)));

			currentWaterGrid[x * waterSize + z] = glm::vec3(x, waterHeight + height, z);
			previousWaterGrid[x * waterSize + z] = currentWaterGrid[x * waterSize + z];
		}
	}

	int index = 0;

	for (int x = 0; x < waterSize - 1; x++)
	{
		for (int z = 0; z < waterSize - 1; z++)
		{
			int offset = x * waterSize + z;
			waterIndices[index] = (short)(offset + 0);
			waterIndices[index + 1] = (short)(offset + 1);
			waterIndices[index + 2] = (short)(offset + waterSize);
			waterIndices[index + 3] = (short)(offset + 1);
			waterIndices[index + 4] = (short)(offset + waterSize + 1);
			waterIndices[index + 5] = (short)(offset + waterSize);
			index += 6;
		}
	}
}

void WaterSceneDrawingProgram::UpdateGridHeights()
{
	float newHeight = 0.0f;

	float heightLeft = 0.0f;
	float heightTop = 0.0f;
	float heightRight = 0.0f;
	float heightBottom = 0.0f;

	int indexOffset = 0;

	// Update height for the vertex inside the grid
	for (int x = 1; x < waterSize - 1; x++)
	{
		for (int z = 1; z < waterSize - 1; z++)
		{
			indexOffset = x * waterSize + z;

			// Get the neighbours previous height
			heightLeft = previousWaterGrid[indexOffset - waterSize].y;
			heightTop = previousWaterGrid[indexOffset + 1].y;
			heightRight = previousWaterGrid[indexOffset + waterSize].y;
			heightBottom = previousWaterGrid[indexOffset - 1].y;

			newHeight = 0.0f;

			// Define the height to add to the current vertex
			newHeight += (((heightLeft + heightTop + heightRight + heightBottom) / 4) - previousWaterGrid[indexOffset].y) * waterSpeed;

			currentWaterGrid[indexOffset].y += newHeight;
		}
	}

	// Update height for the vertex on the top of the grid
	for (int x = 1; x < waterSize - 1; x++)
	{
		indexOffset = x * waterSize + (waterSize - 1);

		// Get the neighbours previous height
		heightLeft = previousWaterGrid[indexOffset - waterSize].y;
		heightRight = previousWaterGrid[indexOffset + waterSize].y;
		heightBottom = previousWaterGrid[indexOffset - 1].y;

		newHeight = 0.0f;

		// Define the height to add to the current vertex
		newHeight += (((heightLeft + heightRight + heightBottom) / 3) - previousWaterGrid[indexOffset].y) * waterSpeed;

		currentWaterGrid[indexOffset].y += newHeight;
	}

	// Update height for the vertex on the bottom of the grid
	for (int x = 1; x < waterSize - 1; x++)
	{
		indexOffset = x * waterSize;

		// Get the neighbours previous height
		heightLeft = previousWaterGrid[indexOffset - waterSize].y;
		heightRight = previousWaterGrid[indexOffset + waterSize].y;
		heightTop = previousWaterGrid[indexOffset + 1].y;

		newHeight = 0.0f;

		// Define the height to add to the current vertex
		newHeight += (((heightLeft + heightRight + heightTop) / 3) - previousWaterGrid[indexOffset].y) * waterSpeed;

		currentWaterGrid[indexOffset].y += newHeight;
	}

	// Update height for the vertex on the left of the grid
	for (int x = 1; x < waterSize - 1; x++)
	{
		indexOffset = x;

		// Get the neighbours previous height
		heightRight = previousWaterGrid[indexOffset + waterSize].y;
		heightTop = previousWaterGrid[indexOffset + 1].y;
		heightBottom = previousWaterGrid[indexOffset - 1].y;

		newHeight = 0.0f;

		// Define the height to add to the current vertex
		newHeight += (((heightTop + heightRight + heightBottom) / 3) - previousWaterGrid[indexOffset].y) * waterSpeed;

		currentWaterGrid[indexOffset].y += newHeight;
	}

	// Update height for the vertex on the right of the grid
	for (int x = 1; x < waterSize - 1; x++)
	{
		indexOffset = waterSize * waterSize - waterSize + x;

		// Get the neighbours previous height
		heightLeft = previousWaterGrid[indexOffset - waterSize].y;
		heightTop = previousWaterGrid[indexOffset + 1].y;
		heightBottom = previousWaterGrid[indexOffset - 1].y;

		newHeight = 0.0f;

		// Define the height to add to the current vertex
		newHeight += (((heightLeft + heightTop + heightBottom) / 3) - previousWaterGrid[indexOffset].y) * waterSpeed;

		currentWaterGrid[indexOffset].y += newHeight;
	}

	// Update the bottom left corner of the grid
	{
		indexOffset = 0;

		// Get the neighbours previous height
		heightRight = previousWaterGrid[indexOffset + waterSize].y;
		heightTop = previousWaterGrid[indexOffset + 1].y;

		newHeight = 0.0f;

		// Define the height to add to the current vertex
		newHeight += (((heightTop + heightRight) / 2) - previousWaterGrid[indexOffset].y) * waterSpeed;

		currentWaterGrid[indexOffset].y += newHeight;
	}

	// Update the top left corner of the grid
	{
		indexOffset = waterSize - 1;

		// Get the neighbours previous height
		heightRight = previousWaterGrid[indexOffset + waterSize].y;
		heightBottom = previousWaterGrid[indexOffset - 1].y;

		newHeight = 0.0f;

		// Define the height to add to the current vertex
		newHeight += (((heightBottom + heightRight) / 2) - previousWaterGrid[indexOffset].y) * waterSpeed;

		currentWaterGrid[indexOffset].y += newHeight;
	}

	// Update the bottom right corner of the grid
	{
		indexOffset = waterSize * waterSize - waterSize;

		// Get the neighbours previous height
		heightLeft = previousWaterGrid[indexOffset - waterSize].y;
		heightTop = previousWaterGrid[indexOffset + 1].y;

		newHeight = 0.0f;

		// Define the height to add to the current vertex
		newHeight += (((heightTop + heightLeft) / 2) - previousWaterGrid[indexOffset].y) * waterSpeed;

		currentWaterGrid[indexOffset].y += newHeight;
	}

	// Update the top right corner of the grid
	{
		indexOffset = waterSize * waterSize - 1;

		// Get the neighbours previous height
		heightLeft = previousWaterGrid[indexOffset - waterSize].y;
		heightBottom = previousWaterGrid[indexOffset - 1].y;

		newHeight = 0.0f;

		// Define the height to add to the current vertex
		newHeight += (((heightBottom + heightLeft) / 2) - previousWaterGrid[indexOffset].y) * waterSpeed;

		currentWaterGrid[indexOffset].y += newHeight;
	}

	// Update the previous grid to the current for the next pass
	for (int x = 0; x < currentWaterGrid.size(); x++)
		previousWaterGrid[x] = currentWaterGrid[x];

	// Reset buffer data (maybe move this part in the draw function)
	glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
	glBufferData(GL_ARRAY_BUFFER, currentWaterGrid.size() * sizeof(glm::vec3), NULL, GL_STREAM_DRAW);
	glBufferData(GL_ARRAY_BUFFER, currentWaterGrid.size() * sizeof(glm::vec3), &currentWaterGrid[0], GL_STREAM_DRAW);
	//glBufferSubData(GL_ARRAY_BUFFER, 0, currentWaterGrid.size() * sizeof(glm::vec3), &currentWaterGrid[0]);
	//glNamedBufferSubData(waterVBO, 0, currentWaterGrid.size() * sizeof(glm::vec3), &currentWaterGrid[0]);
}

void WaterSceneDrawingProgram::PerturbateGrid()
{
	float perturbation = minPerturbationHeight + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (maxPerturbationHeight - minPerturbationHeight)));

	int index = rand() % (waterSize * waterSize);

	previousWaterGrid[index].y += perturbation;

	std::cout << "Perturbation occured at " << index << " and has been raised by " << perturbation << " !\n";
}

void SceneDrawingProgram::Init()
{
	programName = "Scene";

	modelShader.CompileSource(
		"shaders/engine/model_diffuse.vert", 
		"shaders/engine/model_diffuse.frag");
	shaders.push_back(&modelShader);
	const auto jsonText = LoadFile(jsonPath);
	sceneJson = json::parse(jsonText);
	modelNmb = sceneJson["models"].size();
	models.resize(modelNmb);
	positions.resize(modelNmb);
	scales.resize(modelNmb);
	rotations.resize(modelNmb);
	int i = 0;
	for(auto& model : sceneJson["models"])
	{
		const std::string modelName = model["model"];
		if(modelMap.find(modelName) == modelMap.end())
		{
			//Load model
			modelMap[modelName] = Model();
			modelMap[modelName].Init(modelName.c_str());
		}
		models[i] = &modelMap[modelName];
		
		glm::vec3 position;
		position.x = model["position"][0];
		position.y = model["position"][1];
		position.z = model["position"][2];
		positions[i] = position;

		glm::vec3 scale;
		scale.x = model["scale"][0];
		scale.y = model["scale"][1];
		scale.z = model["scale"][2];
		scales[i] = scale;

		glm::vec3 rotation;
		rotation.x = model["angles"][0];
		rotation.y = model["angles"][1];
		rotation.z = model["angles"][2];
		rotations[i] = rotation;
		i++;
	}
	Engine* engine = Engine::GetPtr();
	auto& config = engine->GetConfiguration();
	lastX = config.screenWidth / 2.0f;
	lastY = config.screenHeight / 2.0f;
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

void SceneDrawingProgram::Draw()
{
	Engine* engine = Engine::GetPtr();
	Camera& camera = engine->GetCamera();
	rmt_ScopedOpenGLSample(DrawScene);
	rmt_ScopedCPUSample(DrawSceneCPU, 0);
	glEnable(GL_DEPTH_TEST);
	auto& config = engine->GetConfiguration();

	ProcessInput();

	modelShader.Bind();
	projection = glm::perspective(glm::radians(camera.Zoom), (float)config.screenWidth / (float)config.screenHeight, 0.1f, 100.0f);
	modelShader.SetMat4("projection", projection);
	modelShader.SetMat4("view", camera.GetViewMatrix());
	for(auto i = 0l; i < modelNmb;i++)
	{
		glm::mat4 modelMatrix(1.0f);
		modelMatrix = glm::translate(modelMatrix, positions[i]);
		modelMatrix = glm::scale(modelMatrix, scales[i]);
		auto quaternion = glm::quat(rotations[i]);
		modelMatrix = glm::mat4_cast(quaternion)*modelMatrix;
		modelShader.SetMat4("model", modelMatrix);
		models[i]->Draw(modelShader);
	}
	auto view = camera.GetViewMatrix();
	skybox.SetViewMatrix(view);
	skybox.SetProjectionMatrix(projection);
	skybox.Draw();
}

void SceneDrawingProgram::Destroy()
{
}

void SceneDrawingProgram::UpdateUi()
{
	ImGui::Separator();
}

void SceneDrawingProgram::ProcessInput()
{
	Engine* engine = Engine::GetPtr();
	auto& inputManager = engine->GetInputManager();
	auto& camera = engine->GetCamera();
	float dt = engine->GetDeltaTime();
	float cameraSpeed = 1.0f;

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
#endif

	auto mousePos = inputManager.GetMousePosition();

	camera.ProcessMouseMovement(mousePos.x, mousePos.y, true);

	camera.ProcessMouseScroll(inputManager.GetMouseWheelDelta());
}

int main(int argc, char** argv)
{
	Engine engine;
	auto& config = engine.GetConfiguration();
	config.screenWidth = 1024;
	config.screenHeight = 1024;
	config.windowName = "Hello Water";
	config.bgColor.r = 255;
	config.bgColor.g = 255;
	config.bgColor.b = 255;
	engine.AddDrawingProgram(new SceneDrawingProgram());
	engine.AddDrawingProgram(new WaterSceneDrawingProgram());

	engine.Init();
	engine.GameLoop();

	return EXIT_SUCCESS;
}

