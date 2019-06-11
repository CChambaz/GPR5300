
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{ 
	vec4 position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	gl_Position = position;
	TexCoord = aTexCoords;
}  