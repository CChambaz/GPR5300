#pragma once
#include <vector>

class Plane
{
public:
	void Init();
	void Draw() const;
private:
	std::vector<float> vertices;
	unsigned quadVAO;
	unsigned quadVBO;
};

class Cube
{
public:
	void Init();
	void Draw();
private:
	std::vector<float> vertices;
	unsigned cubeVAO;
	unsigned cubeVBO;
};

class Sphere
{
public:
    void Init();
    void Draw();
private:
    std::vector<float> vertices;
	std::vector<unsigned int> indices;
    unsigned sphereVAO = 0;
    unsigned sphereVBO = 0;
    unsigned sphereEBO = 0;

    unsigned int indexCount;

};

class Grid
{
public:
	void Init(int size);
	void Draw();
private:
	std::vector<glm::vec3> vertices;
	std::vector<short> indices;
	unsigned gridVAO = 0;
	unsigned gridVBO = 0;
	unsigned gridEBO = 0;
};
