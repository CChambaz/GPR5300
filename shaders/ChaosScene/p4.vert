
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec4 FragPos;
out vec3 vertexPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float amount;
uniform float height;
uniform float timeSinceStart;

void main()
{ 
	float y = sin(timeSinceStart * cos(aPos.x * aPos.z * amount)) * height;
	vertexPos = vec4(aPos.x, y, aPos.z, 1.0).xyz;
	FragPos = projection * view * model * vec4(aPos.x, y, aPos.z, 1.0);
	gl_Position = FragPos;
}  