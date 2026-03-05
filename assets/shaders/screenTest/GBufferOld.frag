#version 460 core

layout(location = 0) out vec4 GBuffer_Albedo;
layout(location = 1) out vec4 GBuffer_RoughnessMetallicLinearDepth;

in vec2 uv;
in vec3 fragPos;
in vec3 normal;
in vec3 cameraPos;
in vec4 viewPos;

uniform sampler2D AlbedoSamp;
uniform sampler2D RoughnessSamp;
uniform sampler2D MetallicSamp;
void main() {
	vec4 albedo = texture(AlbedoSamp, uv);
	GBuffer_Albedo = vec4(albedo.rgb, 1.0f);
//	GBuffer_Albedo = vec4(vec3(0.5f), 1.0f);
	float metallic = texture(MetallicSamp, uv).r;
	float roughness = texture(RoughnessSamp, uv).r;
	float linearDepth = viewPos.w;
	GBuffer_RoughnessMetallicLinearDepth = vec4(roughness, metallic, linearDepth, 1.0f);
}