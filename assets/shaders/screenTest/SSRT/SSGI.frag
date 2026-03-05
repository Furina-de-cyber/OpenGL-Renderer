#version 460 core

layout(location = 0) out vec4 SSGIResult;

uniform sampler2D HiZ;
uniform sampler2D normal;
uniform sampler2D RMLD;
uniform sampler2D SSGIInput;
uniform sampler2D albedo;

uniform float far;
uniform float near;

uniform int NumSteps; // 12
uniform int NumRays;  // 12;
uniform int Width;
uniform int Height;
uniform int frameIndexMod8;

uniform mat4 cameraInvView;

in vec2 uv;

layout(std140) uniform RectLight
{
	vec4 LightPosition;
	vec4 LightDirection;    // local z
	vec4 LightColor;
	float LightIntensity;
	int LightType;
	float LightFallOffExp;
    float LightRadius;
	vec4 LightRight;        // local y
	float LightHalfWidth;
	float LightHalfHeight;
	float pad1, pad2;
};

layout(std140) uniform camera
{
	mat4 projection;
	mat4 view;
	vec4 position;
};

const float roughnessThreshold = 6.0f;
const float PI = 3.14159265f;
const int traceStepGroup = 4;
const float roughnessSkipValue = 0.10f; // 0.10f;
//const float roughnessSkipValue = 0.33f;

float GetRoughnessFade(in float Roughness, float thresholdParam)
{
	// mask SSR to reduce noise and for better performance, roughness of 0 should have SSR, at MaxRoughness we fade to 0
	return min(2.0f - Roughness * thresholdParam, 1.0);
}

vec4 calcViewPosition(vec2 sampUV, float zview, float proj00, float proj11){
	vec2 clipXY = sampUV * 2.0f - 1.0f;
	return vec4(clipXY  * ( - zview) / vec2(proj00, proj11), zview, 1.0f);
}

float InterleavedGradientNoise(vec2 uv, float FrameId)
{
	// magic values are found by experimentation
	uv += FrameId * (vec2(47.0f, 17.0f) * 0.695f);

    const vec3 magic = vec3( 0.06711056f, 0.00583715f, 52.9829189f );
    return fract(magic.z * fract(dot(uv, magic.xy)));
}

uvec3 Rand3DPCG16(ivec3 p)
{
    uvec3 v = uvec3(p);

    v = v * 1664525u + 1013904223u;

    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;

    v ^= (v >> 16u);

    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;

    return (v >> 16u);
}

mat3 GetTangentBasis(vec3 TangentZ)
{
    float Sign = (TangentZ.z >= 0.0) ? 1.0 : -1.0;
    float a = -1.0 / (Sign + TangentZ.z);
    float b = TangentZ.x * TangentZ.y * a;
    vec3 TangentX = vec3(
        1.0 + Sign * a * (TangentZ.x * TangentZ.x),
        Sign * b,
        -Sign * TangentZ.x
    );
    vec3 TangentY = vec3(
        b,
        Sign + a * (TangentZ.y * TangentZ.y),
        -TangentZ.y
    );
    return mat3(TangentX, TangentY, TangentZ);
}

vec3 importanceSampleCosineWeighted(vec2 random){
    float u1 = random.x;
    float u2 = random.y;
    float theta = acos(sqrt(u1));
    float phi = 2 * PI * u2;

    float x = sin(theta) * cos(phi);
    float y = sin(theta) * sin(phi);
    float z = cos(theta);
    return vec3(x, y, z);
}

vec2 UniformSampleDisk( vec2 E )
{
	float Theta = 2 * PI * E.x;
	float Radius = sqrt( E.y );
	return Radius * vec2( cos( Theta ), sin( Theta ) );
}

uint ReverseBits32(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return bits;
}

vec2 Hammersley16(uint Index, uint NumSamples, uvec2 Random)
{
    float E1 = fract(float(Index) / float(NumSamples) + float(Random.x) * (1.0 / 65536.0));

    uint rev = ReverseBits32(Index);
    uint vdc16 = (rev >> 16u) ^ (Random.y & 0xFFFFu);
    float E2 = float(vdc16) * (1.0 / 65536.0);

    return vec2(E1, E2);
}

float GetStepScreenFactorToClipAtScreenEdge(vec2 RayStartScreen, vec2 RayStepScreen)
{
	const float RayStepScreenInvFactor = 0.5 * length(RayStepScreen);
	const vec2 S = 1 - max(abs(RayStepScreen + RayStartScreen * RayStepScreenInvFactor) - RayStepScreenInvFactor, 0.0f) / abs(RayStepScreen);
	const float RayStepFactor = min(S.x, S.y) / RayStepScreenInvFactor;

	return RayStepFactor;
}

bool validUV(vec2 uv){
	return uv == clamp(uv, vec2(0.0f), vec2(1.0f));
}

void main() {
	// load basic data from texture
	float roughness = 1.0f;
	vec3 N = texture(normal, uv).rgb;
	N = normalize(N);
	float linearDepth = texture(RMLD, uv).b;
	vec4 albedoColor = texture(albedo, uv);

	// basic space info
	vec2 pixelPosition = uv * vec2(Width, Height);
	vec4 viewPosition = calcViewPosition(uv, linearDepth, projection[0][0], projection[1][1]);
	float deviceZRebuild = - far * near / (far - near) / linearDepth;
	vec4 worldPosition = cameraInvView * viewPosition;
	vec3 V = normalize(position.xyz - worldPosition.xyz);

	vec2 Noise;
	Noise.x = InterleavedGradientNoise(pixelPosition, frameIndexMod8);
	Noise.y = InterleavedGradientNoise(pixelPosition, frameIndexMod8 * 117);
	uvec2 random = Rand3DPCG16(ivec3(ivec2(pixelPosition), frameIndexMod8)).xy;

	mat3 TangentBasis = GetTangentBasis( N );
	vec3 TangentV = transpose(TangentBasis) * V;

	uint traceNumSteps = NumSteps;
	uint traceNumRays = NumRays;

	int traceNumGroups = int((traceNumSteps + traceStepGroup - 1) / traceStepGroup);

	vec4 test = vec4(0.0f);
	vec4 colorSum = vec4(0.0f);
	for(int i = 0; i < traceNumRays; i++){
//		float StepOffset = (Noise.x - 0.5f) / float(traceNumSteps);
		float StepOffset = 0.0f;
		vec2 E = Hammersley16(i, traceNumRays, random);
		vec3 H = TangentBasis * importanceSampleCosineWeighted(E);
		H = normalize(H);
		vec3 L = 2 * dot( V, H ) * H - V;
		L = normalize(L);
		float NoL = clamp(dot(N, L), 0.00001f, 1.0f);          //  lambertian cos in \int cos\theta d\omega
		float NoH = clamp(dot(N, H), 0.00001f, 1.0f);
		float VoH = clamp(dot(V, H), 0.00001f, 1.0f);
		float pdf = NoH / (4 * PI * VoH);

		vec4 viewL = view * vec4(L, 0.0f); // towards -z
		float stepDistanceSafetyToCam = -linearDepth / abs(viewL.z);
		float stepDistance = viewL.z <= 0.0f ? -linearDepth : stepDistanceSafetyToCam;

		vec4 rayStartViewPosition = viewPosition;
		vec4 rayEndViewPosition = viewPosition + vec4(viewL.xyz * stepDistance, 0.0f);

		vec4 rayStartClipPosition = projection * rayStartViewPosition;
		vec4 rayEndClipPosition = projection * rayEndViewPosition;

		vec2 rayStartNDC = rayStartClipPosition.xy / rayStartClipPosition.w;
		vec2 rayEndNDC = rayEndClipPosition.xy / rayEndClipPosition.w;

		vec3 rayStartRefineNDC = vec3(rayStartNDC, - far * near / (far - near) / rayStartViewPosition.z);
		vec3 rayEndRefineNDC   = vec3(rayEndNDC,   - far * near / (far - near) / rayEndViewPosition.z);

//		vec4 rayThicknessEndViewPosition = viewPosition + vec4(0.0f, 0.0f, linearDepth, 0.0f);
		vec4 rayThicknessEndViewPosition = viewPosition + vec4(0, 0, -0.5f, 0);
		vec4 rayThicknessEndClipPosition = projection * vec4(rayThicknessEndViewPosition.xyz, 1.0f);
		vec2 rayThicknessEndNDC = rayThicknessEndClipPosition.xy / rayThicknessEndClipPosition.w;
		vec3 rayThicknessEndRefineNDC = vec3(rayThicknessEndNDC, - far * near / (far - near) / rayThicknessEndViewPosition.z);
	
		vec3 rayStepRefineNDC = rayEndRefineNDC - rayStartRefineNDC;
		float rayClipFactor = GetStepScreenFactorToClipAtScreenEdge(rayStartRefineNDC.xy, rayStepRefineNDC.xy);
		rayStepRefineNDC *= rayClipFactor;

		float tolerance = max(abs(rayStepRefineNDC.z), abs((rayStartRefineNDC.z - rayThicknessEndRefineNDC.z) * 2.0f));

		vec3 rayStep = rayStepRefineNDC / float(traceNumSteps);

		vec2 sampleUV[traceStepGroup];
		vec4 rayDepthDiff = vec4(0.0f);	
		vec4 sampleDepth = vec4(0.0f);
		bvec4 intersect = bvec4(false);
		float hitI = 0.0f;
		bool hit = false;
		float lastDepth = 0.0f;

		vec3 rayStartUV = vec3(0.5 * rayStartRefineNDC.xy + 0.5f, rayStartRefineNDC.z);
		vec3 rayStepUV = vec3(0.5 * rayStep.xy, rayStep.z);
		rayStartUV += StepOffset * rayStepUV;
		float sampleLevel = 0.0f;
		
		for(int j = 0; j < traceNumGroups; j++){
			vec4 sampleLevelArray = vec4(0.0f);
			sampleLevelArray[0] = sampleLevel;
			sampleLevelArray[1] = sampleLevel;
			sampleLevel += 8.0f / float(traceNumSteps) * roughness;
			sampleLevelArray[2] = sampleLevel;
			sampleLevelArray[3] = sampleLevel;
			sampleLevel += 8.0f / float(traceNumSteps) * roughness;
			for(int k = 0; k < traceStepGroup; k++){
				int steps = j * traceStepGroup + k;
				vec3 raySampleUV = rayStartUV + rayStepUV * float(steps);
				sampleUV[k] = raySampleUV.xy;
				sampleDepth[k] = textureLod(HiZ, raySampleUV.xy, sampleLevelArray[k]).r;
				rayDepthDiff[k] = raySampleUV.z - sampleDepth[k];
			}
			intersect = lessThan(abs(rayDepthDiff + tolerance), vec4(tolerance));
			hit = intersect.x || intersect.y || intersect.z || intersect.w;
			if(hit){
				hitI = float(j);
				break;
			}
			else{
				lastDepth = rayDepthDiff[3];
			}
			
		}
		if(hit){
			float firstHitRayDepth = rayDepthDiff[3];
			float lastUnhitRayDepth = rayDepthDiff[2];
			float hitTime = 3.0f;
			if(intersect[2]){
				firstHitRayDepth = rayDepthDiff[2];
				lastUnhitRayDepth = rayDepthDiff[1];
				hitTime = 2.0f;
			}
			if(intersect[1]){
				firstHitRayDepth = rayDepthDiff[1];
				lastUnhitRayDepth = rayDepthDiff[0];
				hitTime = 1.0f;
			}
			if(intersect[0]){
				firstHitRayDepth = rayDepthDiff[0];
				lastUnhitRayDepth = lastDepth;
				hitTime = 0.0f;
			}
			
			float totalHitTime = 4.0f * hitI + hitTime + lastUnhitRayDepth / (lastUnhitRayDepth - firstHitRayDepth);
			vec2 finalHitUV = rayStartUV.xy + rayStepUV.xy * totalHitTime;
			float finalHitZ = rayStartUV.z + rayStepUV.z * totalHitTime;
			if(!validUV(finalHitUV)){
				continue;
			}
			vec3 hitN = texture(normal, finalHitUV).rgb;
			float hitZ = textureLod(HiZ, finalHitUV, 2.0f).r;
			bool zValid = abs(finalHitZ - hitZ) < 0.1f;
			vec4 hitColor = dot(hitN, L) < 0.0f && zValid ? texture(SSGIInput, finalHitUV) : vec4(0.0f);
			
			colorSum += hitColor * NoL / pdf;
		}
	}
	
//	vec4 outputColor = colorSum / max(float(traceNumRays), 0.00001f) * albedoColor / PI;
	vec4 outputColor = colorSum / max(float(traceNumRays), 0.00001f);
	SSGIResult = vec4(outputColor.rgb, 1.0f);
}