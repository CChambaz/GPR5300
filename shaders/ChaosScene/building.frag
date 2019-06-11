
layout(location = 0) out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D activeTexture;

void main()
{
	FragColor = texture(activeTexture, TexCoord);
}

