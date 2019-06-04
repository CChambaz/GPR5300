
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normals;
out vec4 Position;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float speed;
uniform float amount;
uniform float height;
uniform float timeSinceStart;
void main()
{
	/*float xx = (aPos.x-3) * (aPos.x-3);
	float yy = (aPos.y+1) * (aPos.y+1);*/

	//float y = sin(timeSinceStart * speed + (aPos.x * aPos.z * amount) + 0.5 * cos(aPos.x * aPos.z * amount)) * height;
    TexCoords = aTexCoords;    
	Position = model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
}