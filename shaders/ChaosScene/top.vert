
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

	//float amplitude = clamp(tan(angle) / (tan(actualAngle)), 0.1, 1);
	float amplitude = clamp((cos(actualAngle) * sin(actualAngle)) / (cos(angle) * sin(angle)), 0.1, 1);

	// Sinus function
	float sinus = amplitude * sin(speed * actualAngle * timeSinceStart) + distanceToCenter;

	// Circle function
	//float y = (height * amplitude) / distanceToCenter;//clamp(distanceToCenter * sin(actualAngle), 0.1, height);
	float y = clamp(angle * sin(sinus), 0.1, height);
	//float y = (aPos.x * actualAngle) * timeSinceStart;//asin(aPos.x / aPos.z * sinus);
	//float y = timeSinceStart * pow(speed, actualAngle);
	//float y = aPos.x * actualAngle * timeSinceStart;
	//float y = speed * sin(timeSinceStart) * tan(actualAngle);
	vertexPos = vec4(aPos.x, y, aPos.z, 1.0).xyz;
	FragPos = projection * view * model * vec4(aPos.x, y, aPos.z, 1.0);
	gl_Position = FragPos;
}  