
layout(location = 0) out vec4 FragColor;

in vec4 FragPos;
in vec3 vertexPos;

uniform vec3 vertexColor;
uniform vec3 viewPos;

void main()
{
	FragColor = vec4((vertexColor.xyz * sin(clamp(vertexPos.y, 0.1, 1.0))), 1);
	//vec4 color = vec4(1, 1, 1, 0) * clamp(sin(vertexPos.x * vertexPos.z + cos(vertexPos.x * vertexPos.z ) * 0.25), 0, 1);

	//vec4 color = vec4(1, 1, 1, 0) * clamp(sin(vertexPos.x * vertexPos.z + cos(vertexPos.x * vertexPos.z ) * 0.25), 0, 1);
	//FragColor = color;
}

