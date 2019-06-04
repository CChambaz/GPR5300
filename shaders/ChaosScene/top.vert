
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec4 FragPos;
out vec3 vertexPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 center;
uniform float angle;
uniform float speed;
uniform float amount;
uniform float height;
uniform float timeSinceStart;

void main()
{ 
	// Horizontal vector of the current circle
	vec2 horizontal;
	horizontal.x = center.x + length(aPos.xz);
	horizontal.y = center.y;

	// Absolut vector of the current position
	vec2 centerToPoint = aPos.xz - center;

	float distanceToCenter = distance(center, aPos.xz);

	// Define the actual angle
	float actualAngle = atan(distanceToCenter / distance(center, horizontal));

	float amplitude = clamp(tan(angle) / (tan(actualAngle)), 0.1, 1);

	// Sinus function
	float sinus = amplitude * sin(speed * actualAngle * timeSinceStart) + distanceToCenter;

	// Circle function
	float y = clamp(angle * sin(sinus), 0.1, height);
	//float y = asin(aPos.x / aPos.z * sinus);//(aPos.x * actualAngle) * sin(sinus);
	//float y = sinus * pow(speed, actualAngle);
	//float y = aPos.x * actualAngle * sin(sinus);
	//float y = speed * sin(sinus) * tan(angle);
	vertexPos = vec4(aPos.x, y, aPos.z, 1.0).xyz;
	FragPos = projection * view * model * vec4(aPos.x, y, aPos.z, 1.0);
	gl_Position = FragPos;
}  