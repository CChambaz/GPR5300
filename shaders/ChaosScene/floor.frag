
layout(location = 0) out vec4 FragColor;

in vec4 FragPos;
in vec3 vertexPos;

uniform vec3 vertexColor;

void main()
{
	FragColor = vec4((vertexColor.xyz * sin(clamp(vertexPos.y, 0.1, 1.0))), 1);
}

