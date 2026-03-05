#version 460 core

in vec2 uv;

uniform mat4 currentCameraView;
uniform mat4 historyCameraView;
uniform mat4 currentCameraInvView;
uniform mat4 historyCameraInvView;
uniform mat4 cameraProjection;
uniform vec2 invSize;

uniform sampler2D RoughnessMetallicLinearDepthSamp;
uniform sampler2D historyTAAInput;
uniform sampler2D currentTAAInput;

layout(location = 0) out vec4 AAResult;

vec4 calcViewPosition(vec2 sampUV, float zview, float proj00, float proj11){
	vec2 clipXY = sampUV * 2.0f - 1.0f;
	return vec4(clipXY  * ( - zview) / vec2(proj00, proj11), zview, 1.0f);
}

void main() {
	// lack of depth judgement
	vec3 currentValue = texture(currentTAAInput, uv).rgb;
	float currentViewZ = texture(RoughnessMetallicLinearDepthSamp, uv).z;
	vec4 currentViewPosition = calcViewPosition(uv, currentViewZ, cameraProjection[0][0], cameraProjection[1][1]);
	vec4 worldPosition = currentCameraInvView * currentViewPosition;
	vec4 historyViewPosition = historyCameraView * worldPosition;
	vec4 historyClipPosition = cameraProjection * historyViewPosition;

	vec3 historyUVUnlinearZ = historyClipPosition.xyz / historyClipPosition.w;
	historyUVUnlinearZ.xy = historyUVUnlinearZ.xy * 0.5f + 0.5f;

	vec2 reprojectedUVThreshold = vec2(2.5f, 2.5f) * invSize;
	bool uvLegal = historyUVUnlinearZ.x >= 0.0f && historyUVUnlinearZ.y >= 0.0f && historyUVUnlinearZ.x <= 1.0f && historyUVUnlinearZ.y <= 1.0f;
	uvLegal = uvLegal && (historyClipPosition.w > 0.0f);
	bool uvAllowed = abs(historyUVUnlinearZ.x - uv.x) <= reprojectedUVThreshold.x && abs(historyUVUnlinearZ.y - uv.y) <= reprojectedUVThreshold.y;
	vec3 TAA = uvLegal && uvAllowed ? mix(textureLod(historyTAAInput, historyUVUnlinearZ.xy, 0.0f).rgb, currentValue, 0.10f) : currentValue;
//	vec3 TAA = currentValue;
	AAResult = vec4(TAA, 1.0f);
}