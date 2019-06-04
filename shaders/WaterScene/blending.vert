
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec4 FragPos;
out vec3 vertexPos;
out vec4 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float speed;
uniform float amount;
uniform float height;
uniform float timeSinceStart;

void main()
{ 
	/*vec4 color = vec4(0.001, 1, 1, 0.9);
	color *= sin(clamp(vertexPos.y, 0.3, 1.0));
	*/
	/*float y = sin(timeSinceStart * speed + (aPos.x * aPos.z * amount) + 0.5 * cos(aPos.x * aPos.z * amount)) * height;*/

	vec4 worldPos = model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	
	//vertexColor = color;
	vertexPos = worldPos.xyz;
	FragPos = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	gl_Position = FragPos;
}  