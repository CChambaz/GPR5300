
layout(location = 0) out vec4 FragColor;

in vec4 FragPos;
in vec3 vertexPos;
in vec4 vertexColor;

uniform sampler2D reflectionMap;//clipspace
uniform sampler2D refractionMap;//clipspace
uniform sampler2D depthMap;//clipspace

uniform vec3 viewPos;
uniform float refractionStrengh = 0.3f;
uniform float indecesOfRefractionRatio;
uniform float waveStrength;

float GetFresnelDelta(vec3 normal, vec3 eye, float eta)
{
	float angle = 1.0f - clamp(dot(normal, eye), 0.0f, 1.0f);
	float fresnel = angle * angle;
	fresnel = fresnel * fresnel;
	fresnel = fresnel * angle;
	return clamp(fresnel * (1.0f - clamp(length(refract(eye, normal, eta)), 0.0f, 1.0f)) + 0.5f - refractionStrengh, 0.0f, 1.0f);
}

void main()
{
	vec4 color = vec4(0.25, 0.25, 0.75, 0.9);
	color *= sin(clamp(vertexPos.y, 0.1, 1.0));
	FragColor = color;

	/*vec3 normal = normalize(cross(dFdx(vertexPos), dFdy(vertexPos)));

	float refractiveFactor = GetFresnelDelta(normal, normalize(vertexPos.xyz - viewPos), indecesOfRefractionRatio);
	vec2 ndc = (FragPos.xy/FragPos.w)/2.0 + 0.5;

	/*float depth = linearize(texture(depthMap, ndc).r, 0.1, 100.0);
	float depthAttenuation = clamp(abs(depth-vertexPos.z), 0.0,1.0);
	ndc += depthAttenuation * waveStrength;
	ndc = clamp(ndc, 0.001, 0.999);*/

	/*vec4 reflectionColor = texture(reflectionMap, vec2(1.0-ndc.x, ndc.y));

	vec4 refractionColor = texture(refractionMap, ndc);

	vec4 mixedColor = mix(reflectionColor, refractionColor, refractiveFactor);

	FragColor *= mixedColor;*/
}

