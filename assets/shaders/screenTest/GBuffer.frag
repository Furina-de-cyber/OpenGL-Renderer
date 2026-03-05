#version 460 core

layout(location = 0) out vec4 GBuffer_Albedo;
layout(location = 1) out vec4 GBuffer_RoughnessMetallicLinearDepth;
layout(location = 2) out vec4 GBuffer_Normal;
layout(location = 3) out float GBuffer_DeviceZ;

in vec2 uv;
in vec3 normal;
in vec4 clipPosition;

uniform sampler2D AlbedoSamp;
uniform sampler2D RoughnessSamp;
uniform sampler2D MetallicSamp;

uniform float far;
uniform float near;

void main() {
	vec4 albedo = texture(AlbedoSamp, uv);
	GBuffer_Albedo = vec4(albedo.rgb, 1.0f);
	float metallic = texture(MetallicSamp, uv).b;
	float roughness = texture(RoughnessSamp, uv).g;
	float linearDepth = - clipPosition.w;
	GBuffer_RoughnessMetallicLinearDepth = vec4(roughness, metallic, linearDepth, 1.0f);
	GBuffer_Normal = vec4(normal, 1.0f);

//	float reverseDeviceZ = far * near / (linearDepth * (far - near)); 
	float reverseDeviceZ = far * near / (clipPosition.w * (far - near)); 
	GBuffer_DeviceZ = reverseDeviceZ;
}