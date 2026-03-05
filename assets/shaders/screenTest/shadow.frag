#version 460 core

layout(location = 0) out float shadowFactor;

layout(std140) uniform camera
{
	mat4 projection;
	mat4 view;
	vec4 position;
};

in vec2 uv;

uniform vec4 lightPosition;
uniform vec4 lightAngleInfo; // x is tan, y is inv tan, z is max of light width and height, w is 0.0f
uniform mat4 lightViewMatrix;   // LH
uniform mat4 lightProjMatrix;
uniform mat4 cameraInvView;
uniform vec2 invShadowMapSize;
uniform vec2 lightFN; // x is far, y is near
uniform int searchSampleNum;
uniform int occSampleNum;
uniform int frameIndexMod8;

uniform sampler2D RoughnessMetallicLinearDepthSamp;
uniform sampler2D shadowProjSamp;

mat4 postProcessMatrix(float near){
	return mat4(
			vec4(0.5f, 0.0f, 0.0f, 0.0f),
			vec4(0.0f, 0.5f, 0.0f, 0.0f),
			vec4(0.0f, 0.0f, 1.0f / near, 0.0f),
			vec4(0.5f, 0.5f, 0.0f, 1.0f)
			);
}
vec4 calcViewPosition(vec2 sampUV, float zview, float proj00, float proj11){
	vec2 clipXY = sampUV * 2.0f - 1.0f;
	return vec4(clipXY  * ( - zview) / vec2(proj00, proj11), zview, 1.0f);
}

uvec2 SobolIndex(uvec2 Base, uint Index, int Bits) // Bits = 10
{
    const uvec2 SobolNumbers[10] = uvec2[10](
        uvec2(0x8680u, 0x4c80u), uvec2(0xf240u, 0x9240u), uvec2(0x8220u, 0x0e20u), uvec2(0x4110u, 0x1610u), uvec2(0xa608u, 0x7608u),
        uvec2(0x8a02u, 0x280au), uvec2(0xe204u, 0x9e04u), uvec2(0xa400u, 0x4682u), uvec2(0xe300u, 0xa74du), uvec2(0xb700u, 0x9817u)
    );

    uvec2 Result = Base;

    for (int b = 0; b < 10 && b < Bits; ++b)
    {
        if ((Index & (1u << b)) != 0u)
        {
            Result ^= SobolNumbers[b];
        }
    }
    return Result;
}

vec2 UniformSampleDiskConcentricApprox( vec2 E )
{
	vec2 sf = E * sqrt(2.0) - sqrt(0.5);
	vec2 sq = sf*sf;
	float root = sqrt(2.0*max(sq.x, sq.y) - min(sq.x, sq.y));
	if (sq.x > sq.y)
	{
		sf.x = sf.x > 0 ? root : -root;
	}
	else
	{
		sf.y = sf.y > 0 ? root : -root;
	}
	return sf;
}

mat2 GenerateScale2x2Matrix(vec2 majorAxis, float majorScale, float minorScale){
	return mat2(
		vec2(
		(majorScale-minorScale)*majorAxis.x*majorAxis.x+minorScale, 
		(majorScale-minorScale)*majorAxis.x*majorAxis.y
		),
		vec2(
		(majorScale-minorScale)*majorAxis.x*majorAxis.y,
		(majorScale-minorScale)*majorAxis.y*majorAxis.y+minorScale 
		)
	);
}

mat2 AnisotropyMatrix(vec2 shadowUV, float searchRadius, out float test){
	vec2 shadowUVDx = dFdx(shadowUV);
	vec2 shadowUVDy = dFdy(shadowUV);
	vec2 anisotropyVector = normalize(shadowUVDx + shadowUVDy);

	vec2 anisotropyFactorVector = sqrt(vec2(shadowUVDx.x*shadowUVDx.x+shadowUVDy.x*shadowUVDy.x,
		shadowUVDx.y*shadowUVDx.y + shadowUVDy.y*shadowUVDy.y));
	float majorFactor = max(anisotropyFactorVector.x, anisotropyFactorVector.y);
	float minorFactor = min(anisotropyFactorVector.x, anisotropyFactorVector.y);
	float majorScale = clamp(majorFactor / searchRadius, 1.0f, 7.0f);
	float minorScale = clamp(minorFactor / searchRadius, 1.0f, 7.0f);
	test = majorScale;
	return GenerateScale2x2Matrix(anisotropyVector, majorScale, minorScale);
}

float PCF3x3(vec2 centerUV, float bias, vec2 invSize, float depth){
	vec2 pos[9] = {
		vec2(bias*invSize.x,  bias*invSize.y),
		vec2(bias*invSize.x,  0),
		vec2(bias*invSize.x,  -bias*invSize.y),
		vec2(0,               bias*invSize.y),
		vec2(0,               0),
		vec2(0,               -bias*invSize.y),
		vec2(-bias*invSize.x, bias*invSize.y),
		vec2(-bias*invSize.x, 0),
		vec2(-bias*invSize.x, -bias*invSize.y),
	};
	float res = 0.0f;
	for(int i=0;i<9;i++){
		res += textureLod(shadowProjSamp, centerUV+pos[i], 0.0f).r > depth - 0.001f ? 1.0f : 0.0f;
	}
	return res / 9.0f;
}

mat2 staticsElepticalAntiAliasing(float SumWeight, float UVCrossSum, float FilterRadius,
	vec2 OccluderUVGradient, vec2 UVOffsetSquareSum){
	const float EigenThreshold = 1.3;
	const float MinimumCovaranceOccluders = 2;
	float CovarianceMatrixNonDiagonal = SumWeight * UVCrossSum - OccluderUVGradient.x * OccluderUVGradient.y;
	vec2 CovarianceMatrixDiagonal = vec2(
		SumWeight * UVOffsetSquareSum.x - OccluderUVGradient.x * OccluderUVGradient.x,
		SumWeight * UVOffsetSquareSum.y - OccluderUVGradient.y * OccluderUVGradient.y);

	float CovarianceMatrixTrace = CovarianceMatrixDiagonal.x + CovarianceMatrixDiagonal.y;
	float T = sqrt(CovarianceMatrixTrace * CovarianceMatrixTrace -
		4 * (CovarianceMatrixDiagonal.x * CovarianceMatrixDiagonal.y - CovarianceMatrixNonDiagonal * CovarianceMatrixNonDiagonal));
	vec2 EigenValues = 0.5 * (CovarianceMatrixTrace + vec2(T, -T));

	float MaxEigenValue = max(EigenValues.x, EigenValues.y);
	float MinEigenValue = min(EigenValues.x, EigenValues.y);
	vec2 LongestEigenVector = normalize(vec2(MaxEigenValue - CovarianceMatrixDiagonal.y, CovarianceMatrixNonDiagonal));
	float LongestEigenVectorFactor = sqrt(MaxEigenValue / MinEigenValue) - EigenThreshold;
	float CovarianceBasedEllipticalFactor = 8 * LongestEigenVectorFactor;
		
	float CovarianceFade = clamp((SumWeight >= MinimumCovaranceOccluders ? 1 : 0) * LongestEigenVectorFactor, 0.0f, 1.0f);
		
	CovarianceFade *= clamp(1 - 5 * (length(OccluderUVGradient) - 0.5), 0.0f, 1.0f);
	vec2 AverageBasedMajorAxis = normalize(vec2(OccluderUVGradient.y, -OccluderUVGradient.x));
	float AverageBasedEllipticalFactor = length(OccluderUVGradient) * SumWeight;

	float EllipticalFade = 1.0f / 1024.0f / FilterRadius;

	vec2 EllipseMajorAxis = normalize(mix(AverageBasedMajorAxis, LongestEigenVector, CovarianceFade));
	float MaxEllipticalFactor = 6 - 3 * CovarianceFade;
	float EllipticalFactor = min(mix(AverageBasedEllipticalFactor, CovarianceBasedEllipticalFactor, CovarianceFade), MaxEllipticalFactor) * EllipticalFade;

	mat2 ElepticalProjectionMatrix = GenerateScale2x2Matrix(EllipseMajorAxis, EllipticalFactor+1.0f, 1.0f);
	return ElepticalProjectionMatrix;
}

bool include(vec2 aim, vec2 down, vec2 up){
	return aim.x >= down.x && aim.y >= down.y && aim.x <= up.x && aim.y <= up.y;
}

void main(){
	float far  = lightFN.x;
	float near = lightFN.y;

	float viewDepth = textureLod(RoughnessMetallicLinearDepthSamp, uv, 0.0f).b;
	vec4 cameraViewPosition = calcViewPosition(uv, viewDepth, projection[0][0], projection[1][1]);
	mat4 cameraViewToRefineLightClipMatrix = postProcessMatrix(near) * lightProjMatrix * lightViewMatrix * cameraInvView;
	// vec4(0.5*x_view + 0.5*z_view, 0.5*y_view + 0.5*z_view, (f-z_view)/(f-n), z_view);
	vec4 refineLightClipPosition = cameraViewToRefineLightClipMatrix * cameraViewPosition;
	float lightZ = refineLightClipPosition.w;

	float shadowZ = 1.0f - refineLightClipPosition.z; // linear non-reverse z
	vec2 shadowUV = refineLightClipPosition.xy / refineLightClipPosition.w;
	float shadowUnlinearZ = refineLightClipPosition.z / refineLightClipPosition.w;
	float nearestDepth = textureLod(shadowProjSamp, shadowUV, 0.0f).r;

	vec3 cameraViewPositionDDX = dFdx(cameraViewPosition.xyz);
	vec3 cameraViewPositionDDY = dFdy(cameraViewPosition.xyz);
	vec4 refineLightClipPositionDDX = cameraViewToRefineLightClipMatrix * vec4(cameraViewPositionDDX, 0.0f);
	vec4 refineLightClipPositionDDY = cameraViewToRefineLightClipMatrix * vec4(cameraViewPositionDDY, 0.0f);
	vec3 DepthBiasPlaneNormal = cross(refineLightClipPositionDDX.xyz, refineLightClipPositionDDY.xyz);
    float DepthBiasFactor = 1 / max(abs(DepthBiasPlaneNormal.z), length(DepthBiasPlaneNormal) * 0.0872665);
    vec2 DepthBiasDotFactors = DepthBiasPlaneNormal.xy * DepthBiasFactor;

	float searchRadius = 0.5 * lightAngleInfo.z * lightAngleInfo.y / lightZ;
//	uvec2 SobolRandom = SobolIndex(uvec2(gl_FragCoord), 3, 10);
//	uvec2 SobolRandom = uvec2(0x2FA0, 0x1EC6);
	uvec2 SobolRandom = SobolIndex(uvec2(0x2FA0, 0x1EC6), frameIndexMod8, 10);
	float DepthSum = 0;
	float DepthSquaredSum = 0;
	float SumWeight = 0;
	float DiscardedSampleCount = 0;
	vec2 UVOffsetSum = vec2(0, 0);
	vec2 UVOffsetSquareSum = vec2(0, 0);
	float UVCrossSum = 0;
	for (int i = 0; i < searchSampleNum; i++)
	{
		vec2 E = vec2(SobolIndex(SobolRandom ^ uvec2(0x4B05, 0xB0CD), i << 3, 10)) / 65536.0f;
		vec2 PCSSSample = UniformSampleDiskConcentricApprox(E);
		vec2 SampleUVOffset = PCSSSample * searchRadius;
		vec2 SampleUV = shadowUV + SampleUVOffset;
		float ShadowDepth = include(SampleUV, vec2(0.0f), vec2(1.0f)) ? textureLod(shadowProjSamp, SampleUV, 0.0f).r : 100.0f;
		float ShadowDepthCompare = shadowZ - ShadowDepth;
		float SampleDepthBias = max(dot(DepthBiasDotFactors, SampleUVOffset), 0);
//		float SampleDepthBias = 0.01f;
		if (ShadowDepthCompare > SampleDepthBias)
		{
			DepthSum += ShadowDepth;
			DepthSquaredSum += ShadowDepth * ShadowDepth;
			SumWeight += 1;
			vec2 UV = PCSSSample;
			UVOffsetSum += UV;
			UVOffsetSquareSum += UV * UV;
			UVCrossSum += UV.x * UV.y;
		}
	}

	float test = 0.0f;
	mat2 anisotropyMat = AnisotropyMatrix(shadowUV, searchRadius, test);

	float RandomFilterScale = 0.75f;
	float NormalizeFactor = (1 / SumWeight);
	vec2 OccluderUVGradient = UVOffsetSum * NormalizeFactor;
	float DepthAvg = DepthSum * NormalizeFactor;
	float DepthVariance = NormalizeFactor * DepthSquaredSum - DepthAvg * DepthAvg;
	float DepthStandardDeviation = sqrt(DepthVariance);
	float AverageOccluderDistance = shadowZ - DepthAvg;
	float Penumbra = searchRadius * AverageOccluderDistance / DepthAvg;
	float RawFilterRadius = RandomFilterScale * Penumbra;
	float FilterRadius = max(1.0f * invShadowMapSize.x , RawFilterRadius);
//	float FilterRadius = clamp(RawFilterRadius, 1.0f * invShadowMapSize.x, 5.0f * invShadowMapSize.x);

//  meaningless
//	float ElepticalFading = 3.0f / 1024.0f / FilterRadius;
	float ElepticalFading = 1.0f / 1024.0f / FilterRadius;
	float ElepticalFactor = length(OccluderUVGradient) * SumWeight * ElepticalFading;
	vec2 GrandAxisDir = normalize(vec2(OccluderUVGradient.y, -OccluderUVGradient.x));
//	ElepticalFactor = clamp(ElepticalFactor, 1.0f, 1.0f);
//	mat2 ElepticalProjectionMatrix = GenerateScale2x2Matrix(GrandAxisDir, ElepticalFactor+1.0f, 1.0f);

	mat2 ElepticalProjectionMatrix = staticsElepticalAntiAliasing(SumWeight, UVCrossSum, FilterRadius,
		OccluderUVGradient, UVOffsetSquareSum);
//	mat2 PCFUVMatrix = anisotropyMat;
	mat2 PCFUVMatrix = anisotropyMat * ElepticalProjectionMatrix;
//	mat2 PCFUVMatrix = ElepticalProjectionMatrix;
	float VisibleLightAccumulation = 0.0f;
	float num = 0.0f;
	for (int j = 0; j < occSampleNum; j++)
	{
		vec2 E = vec2(SobolIndex(SobolRandom ^ uvec2(0x4B05, 0xB0CD), j << 3, 10)) / 65536.0f;
		vec2 PCFSample = UniformSampleDiskConcentricApprox(E);
		vec2 SampleUVOffset = PCFUVMatrix * PCFSample * FilterRadius;
//		vec2 SampleUVOffset = PCFSample * FilterRadius;
		vec2 SampleUV = shadowUV + SampleUVOffset;
		if(include(SampleUV, vec2(0.0f), vec2(1.0f))){
			float SampleDepth = textureLod(shadowProjSamp, SampleUV, 0.0f).r;
//			float SampleDepthBias = max(dot(DepthBiasDotFactors, SampleUVOffset), 0) + 0.01f;
			float SampleDepthBias = 0.01f;
//			VisibleLightAccumulation += clamp((SampleDepth - shadowZ + SampleDepthBias) * 5.0f + 1.0f, 0.0f, 1.0f);
			VisibleLightAccumulation += shadowZ > SampleDepth + SampleDepthBias ? 0.0f : 1.0f;
			num += 1.0f;
		}
	}

	float Visibility = num == 0.0f? 1.0f : VisibleLightAccumulation / num;
//	shadowFactor = shadowZ > nearestDepth ? 0.0f : 1.0f;
//	shadowFactor = clamp(1.0f - SumWeight / float(searchSampleNum), 0.0f, 1.0f);
	shadowFactor = Visibility;
//	shadowFactor = test;
//	shadowFactor=PCF3x3(shadowUV, 2.0f, invShadowMapSize, shadowZ);
}