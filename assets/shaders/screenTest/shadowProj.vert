#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUv;
layout(location = 2) in vec3 aNormal;

uniform vec4 combineDepthParam; // x: max tan, y: slope of tan, z: const of tan, w: near (namely minZ)
/*
	Axis in lightViewMatrix
	vec3 lightAxisZ = normalize(LightDirection.xyz);
	vec3 lightAxisY = normalize(LightRight.xyz);
	vec3 lightAxisX = cross(lightAxisY, lightAxisZ);
	LightProjMatrix minZ = n maxZ = f
	1,   0,    0,                          0
	0,   1,    0,                          0
	0,   0,    - MinZ / (MaxZ - MinZ),     MaxZ * MinZ / (MaxZ - MinZ)
	0,   0,    1,                          0
	clip space coordinate:
	x_view, y_view, n*(f-z_view)/(f-n), z_view
*/
uniform vec4 lightPosition;
uniform mat4 lightViewMatrix;   // RH
uniform mat4 lightProjMatrix;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;

out float lightDepth;
out float lightBias;

void main()
{
	vec4 worldPosition = modelMatrix * vec4(aPos, 1.0f);
	vec4 lightClipPosition = lightProjMatrix * lightViewMatrix * worldPosition;

	vec3 N = normalize(normalMatrix * aNormal);
	vec3 L = normalize(lightPosition.xyz - worldPosition.xyz);
	float NoL = dot(N, L);

	float tanNL = (abs(NoL) > 0.00001f) ? (sqrt(1 - clamp(NoL*NoL, 0.0f, 1.0f)) / NoL) : combineDepthParam.x;
	tanNL = clamp(tanNL, 0, combineDepthParam.x);

	lightBias = combineDepthParam.y * tanNL + combineDepthParam.z;
	lightDepth = lightClipPosition.z / combineDepthParam.w;
	lightClipPosition.z = lightClipPosition.z * -2.0f + lightClipPosition.w;
	gl_Position = lightClipPosition;// * vec4(1.0f, 1.0f, -1.0f, 1.0f);
}
