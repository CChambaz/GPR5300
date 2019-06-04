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

class ChaosSceneDrawingProgram : public DrawingProgram
{
public:
	void Init() override;
	void Draw() override;
	void Destroy() override;
	void UpdateUi() override;
	void ProcessInput();
private:
	glm::mat4 projection = {};
	float far = 10000.0f;

	Grid grid;
	int gridSize = 250;

	// left attributs
	Shader leftShader;
	float leftSpeed = 7.0f;
	float leftAmount = 10.0f;
	float leftHeight = 0.4f;
	float leftPosition[3] = { (float)(-gridSize / 2),0,(float)(-gridSize / 2) };
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
	float topCenter[2] = { (float)(gridSize / 2), (float)(gridSize / 2) };
	float topAngle = 0.5f;
	float topHeight = 5.0f;
	float topSpeed = 2.0f;
	float topColor[3] = { 0.62,0.37,0.62 };
	float topPositionYOffset = 5;

	// Camera attribut
	float cameraPosition[3] = { 0,(float)(gridSize / 2),0 };

	float minPerturbationHeight = 1.0f;
	float maxPerturbationHeight = 4.0f;
	float noPerturbationSince = 0.0f;
	float perturbationCoolDown = 0.01f;

	bool debugMod = false;
};

void ChaosSceneDrawingProgram::Init()
{
	programName = "Chaos scene";
	Engine* engine = Engine::GetPtr();
	auto& config = engine->GetConfiguration();
	Camera& camera = engine->GetCamera();
	grid.Init(gridSize);

	leftShader.CompileSource(
		"shaders/ChaosScene/floor.vert",
		"shaders/ChaosScene/floor.frag"
	);

	wallShader.CompileSource(
		"shaders/ChaosScene/wall.vert",
		"shaders/ChaosScene/wall.frag"
	);

	topShader.CompileSource(
		"shaders/ChaosScene/top.vert",
		"shaders/ChaosScene/top.frag"
	);

	camera.Position.x = cameraPosition[0];
	camera.Position.y = cameraPosition[1];
	camera.Position.z = cameraPosition[2];
}

void ChaosSceneDrawingProgram::Draw()
{
	/*rmt_ScopedOpenGLSample(DrawWater);
	rmt_ScopedCPUSample(DrawWaterCPU, 0);*/
	Engine* engine = Engine::GetPtr();
	auto& config = engine->GetConfiguration();
	Camera& camera = engine->GetCamera();

	if (!debugMod)
	{
		camera.Position.x = cameraPosition[0];
		camera.Position.y = cameraPosition[1];
		camera.Position.z = cameraPosition[2];
	}

	projection = glm::perspective(glm::radians(camera.Zoom), (float)config.screenWidth / (float)config.screenHeight, 0.1f, far);

	glEnable(GL_DEPTH_TEST);

	ProcessInput();

	// left rendering
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		leftShader.Bind();
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(leftPosition[0] + gridSize, leftPosition[1] + gridSize, leftPosition[2] - (gridSize * 1.25)));
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

		grid.Draw();
	}

	// right rendering
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		leftShader.Bind();
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(leftPosition[0] - gridSize * 0.72, leftPosition[1] + gridSize, leftPosition[2] - (gridSize * 0.57)));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(-45.0f), glm::vec3(0, 0, 1));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(leftScale[0], leftScale[1], leftScale[2]));

		leftShader.SetMat4("model", modelMatrix);
		leftShader.SetMat4("projection", projection);
		leftShader.SetMat4("view", camera.GetViewMatrix());
		leftShader.SetVec3("vertexColor", wallColor);
		leftShader.SetFloat("speed", wallSpeed);
		leftShader.SetFloat("amount", wallAmount);
		leftShader.SetFloat("height", wallHeight);
		leftShader.SetFloat("timeSinceStart", engine->GetTimeSinceInit());

		grid.Draw();
	}

	// Top rendering
	modelMatrix = glm::mat4(1.0f);
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		topShader.Bind();
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(leftPosition[0], leftPosition[1] + gridSize, leftPosition[2] - (gridSize * 1.25)));
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

		grid.Draw();
	}
}

void ChaosSceneDrawingProgram::Destroy()
{
	
}

void ChaosSceneDrawingProgram::UpdateUi()
{
	ImGui::Separator();
	ImGui::SliderFloat("top angle", &wallAmount, 0.0f, 40.0f);
	ImGui::SliderFloat("top speed", &wallHeight, 0.0f, 15.0f);
	ImGui::SliderFloat("top height", &wallSpeed, 0.0f, 10.0f);
	ImGui::SliderFloat("floor amount", &leftAmount, 0.0f, 100.0f);
	ImGui::SliderFloat("floor height", &leftHeight, -10.0f, 10.0f);
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

