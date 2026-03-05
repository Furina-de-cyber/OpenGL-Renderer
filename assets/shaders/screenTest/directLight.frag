#version 460 core

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

uniform sampler2D AlbedoSamp;
uniform sampler2D RoughnessMetallicLinearDepthSamp;
uniform sampler2D NormalSamp;

uniform sampler2D LTCMatSamp;
uniform sampler2D LTCAmpSamp;

uniform sampler2D shadowFactorSamp;

uniform mat4 cameraInvView;

in vec4 clipPosition;

layout(location = 0) out vec4 DirectLight;

const float PI = 3.14159265f;

vec4 calcViewPosition(vec2 sampUV, float zview, float proj00, float proj11){
	vec2 clipXY = sampUV * 2.0f - 1.0f;
	return vec4(clipXY  * ( - zview) / vec2(proj00, proj11), zview, 1.0f);
}

float calcDistanceAttenuation(vec3 toLight, vec3 lightNormal, out vec3 L,
	float radius, float fallOffExp){
	float distSqr = dot(toLight, toLight);
	float dist = sqrt(distSqr);
	float attenuation = dist < radius ? pow((1-distSqr/(radius*radius)), fallOffExp) : 0.0f;
	L = normalize(toLight);
	float backAttenuation = dot(-lightNormal, L) > 0 ? 1.0f : 0.0f;
	return attenuation * backAttenuation;
}

vec3 fresnelSchlick(vec3 F0, float NoL){
	vec3 F90 = clamp(F0 * 50.0f, 0.0f, 1.0f);
	float OneMinusNoL = clamp(1 - NoL, 0.0f, 1.0f);
	return F0 + (F90 - F0) * pow(OneMinusNoL, 5.0f);
}

vec3 fresnelSchlickUE(vec3 F0, vec3 F90, float VoH)
{
	float Fc = pow(1 - VoH, 5);
	return F90 * Fc + (1 - Fc) * F0;
}

float SphereHorizonCosWrap( float NoL, float SinAlphaSqr )
{
	float SinAlpha = sqrt( SinAlphaSqr );
	if( NoL < SinAlpha )
	{
		NoL = max( NoL, -SinAlpha );
		NoL = ( SinAlpha + NoL ) * ( SinAlpha + NoL ) / ( 4 * SinAlpha );
	}
	return NoL;
}

vec3 lambertLightIntegrate(mat3 lightToWorld, vec3 L, vec3 N, vec2 lightHalfSize, out float NoL, out float baseIrradiance){
	mat3 WorldToLight = inverse(lightToWorld);
	vec3 lightSpaceL = - WorldToLight * L;
	vec3 SPSpaceL = vec3(-lightSpaceL.xy, lightSpaceL.z);
	vec3 rect0 = SPSpaceL + vec3(lightHalfSize.x, lightHalfSize.y, 0.0f);
	vec3 rect1 = SPSpaceL + vec3(-lightHalfSize.x, lightHalfSize.y, 0.0f);
	vec3 rect2 = SPSpaceL + vec3(-lightHalfSize.x, -lightHalfSize.y, 0.0f);
	vec3 rect3 = SPSpaceL + vec3(lightHalfSize.x, -lightHalfSize.y, 0.0f);

	rect0 = normalize(rect0);
	rect1 = normalize(rect1);
	rect2 = normalize(rect2);
	rect3 = normalize(rect3);

	vec3 normal01 = normalize(cross(rect0, rect1));
	vec3 normal12 = normalize(cross(rect1, rect2));
	vec3 normal23 = normalize(cross(rect2, rect3));
	vec3 normal30 = normalize(cross(rect3, rect0));

	float angel01 = acos(clamp(dot(rect0, rect1), -1.0f, 1.0f));
	float angel12 = acos(clamp(dot(rect1, rect2), -1.0f, 1.0f));
	float angel23 = acos(clamp(dot(rect2, rect3), -1.0f, 1.0f));
	float angel30 = acos(clamp(dot(rect3, rect0), -1.0f, 1.0f));

	vec3 averageL = normal01 * angel01 + normal12 * angel12 + normal23 * angel23 + normal30 * angel30;
	averageL = averageL * vec3(1.0f, 1.0f, -1.0f);
	vec3 worldAverageL = lightToWorld * averageL;
	float worldAverageLLen = length(worldAverageL);
	baseIrradiance = 0.5 * worldAverageLLen;
	vec3 normalizedWorldAverageL = worldAverageL / worldAverageLLen;
	float SinAlphaSqr = baseIrradiance / PI;
	NoL = SphereHorizonCosWrap( dot( N, normalizedWorldAverageL ), SinAlphaSqr );

	return normalizedWorldAverageL;
}

vec3 getLambertianParam(vec3 albedo, float metallic){
	return albedo * (1 - metallic) / PI;
}

vec3 LTCIntegrate(mat3 worldToLoc, mat3 LTC, vec3 L, vec3 N, vec2 lightHalfSize, 
	out float NoL, out float baseIrradiance, vec3 lightAxisX, vec3 lightAxisY){
//	vec3 rect0 = L + lightHalfSize.x * lightAxisX + lightHalfSize.y * lightAxisY;
//	vec3 rect1 = L - lightHalfSize.x * lightAxisX + lightHalfSize.y * lightAxisY;
//	vec3 rect2 = L - lightHalfSize.x * lightAxisX - lightHalfSize.y * lightAxisY;
//	vec3 rect3 = L + lightHalfSize.x * lightAxisX - lightHalfSize.y * lightAxisY;
	vec3 rect0 = L + lightHalfSize.x * lightAxisX + lightHalfSize.y * lightAxisY;
	vec3 rect1 = L + lightHalfSize.x * lightAxisX - lightHalfSize.y * lightAxisY;
	vec3 rect2 = L - lightHalfSize.x * lightAxisX - lightHalfSize.y * lightAxisY;
	vec3 rect3 = L - lightHalfSize.x * lightAxisX + lightHalfSize.y * lightAxisY;

	mat3 transMatrix = LTC * worldToLoc;
	rect0 = normalize(transMatrix * rect0);
	rect1 = normalize(transMatrix * rect1);
	rect2 = normalize(transMatrix * rect2);
	rect3 = normalize(transMatrix * rect3);

	vec3 normal01 = normalize(cross(rect0, rect1));
	vec3 normal12 = normalize(cross(rect1, rect2));
	vec3 normal23 = normalize(cross(rect2, rect3));
	vec3 normal30 = normalize(cross(rect3, rect0));

	float angel01 = acos(clamp(dot(rect0, rect1), -1.0f, 1.0f));
	float angel12 = acos(clamp(dot(rect1, rect2), -1.0f, 1.0f));
	float angel23 = acos(clamp(dot(rect2, rect3), -1.0f, 1.0f));
	float angel30 = acos(clamp(dot(rect3, rect0), -1.0f, 1.0f));

	vec3 averageL = normal01 * angel01 + normal12 * angel12 + normal23 * angel23 + normal30 * angel30;

	float averageLLen = length(averageL);
	baseIrradiance = averageLLen;
	vec3 normalizedAverageL = averageL / averageLLen;
	float SinAlphaSqr = baseIrradiance;
	NoL = SphereHorizonCosWrap(normalizedAverageL.z, SinAlphaSqr );
//	NoL = dot(N, L);
//	NoL = dot(N, normalizedAverageL);
//	if (averageLLen <= 1e-5 || normalizedAverageL.z <= 0.0)
//	{
//		NoL = 0.0;
//		baseIrradiance = 0.0;
//		return vec3(0.0);
//	}
	return normalizedAverageL;
}

vec3 RectGGXApproxLTC(float Roughness, vec3 F0, vec3 ToLight, vec3 N, 
	vec3 V, vec2 lightHalfSize, vec3 lightAxisX, vec3 lightAxisY)
{
	vec3 F90 = clamp(F0 * 50.0f, 0.0f, 1.0f);

	float NoV = clamp(abs(dot(N, V)) + 1e-5f, 0.0f, 1.0f);

	vec2 UV = vec2( Roughness, sqrt( 1 - NoV ));
	UV = UV * (63.0 / 64.0) + (0.5 / 64.0);
   
	vec4 LTCMat = texture(LTCMatSamp, UV);
	vec4 LTCAmp = texture(LTCAmpSamp, UV);

	mat3 LTC = mat3(
		vec3(LTCMat.x,      0,     LTCMat.y), 
		vec3(	    0,      1,            0),
		vec3(LTCMat.z,      0,     LTCMat.w)
	);
//	float LTCDet = LTCMat.x * LTCMat.w - LTCMat.y * LTCMat.z;
//
//	vec4 InvLTCMat = LTCMat / LTCDet;
//	mat3 InvLTC = mat3(
//		InvLTCMat.w,    0,    -InvLTCMat.y,
//		          0,    1,               0,
//		-InvLTCMat.z ,  0,     InvLTCMat.x 
//	);

	// Rotate to tangent space
	vec3 T1 = normalize( V - N * dot( N, V ) );
	vec3 T2 = cross( N, T1 );
	mat3 locToWorld = mat3( T1, T2, N );
	mat3 worldToLoc = transpose(locToWorld);
	float NoL = 0.0f;
	float baseIrradiance = 0.0f;
	vec3 L = LTCIntegrate(worldToLoc, LTC, ToLight, N, lightHalfSize, NoL, baseIrradiance, lightAxisX, lightAxisY);

	float Irradiance = baseIrradiance * max(NoL, 0.0f);
	vec3 IrradianceScale = F90 * LTCAmp.y + ( LTCAmp.x - LTCAmp.y ) * F0;
	
	return Irradiance * IrradianceScale;
}

float luma(vec3 color) {
    return dot(color, vec3(0.3333, 0.3333, 0.3333));
}

void main() {
	vec2 sampUV = clipPosition.xy / clipPosition.w * 0.5 + 0.5;
	vec4 albedo = texture(AlbedoSamp, sampUV);
	vec4 normal = texture(NormalSamp, sampUV);
	vec4 RoughnessMetallicLinearDepth = texture(RoughnessMetallicLinearDepthSamp, sampUV);
	float roughness = RoughnessMetallicLinearDepth.r;
	float metallic = RoughnessMetallicLinearDepth.g;
	float linearDepth = RoughnessMetallicLinearDepth.b;

	vec4 viewPosition = calcViewPosition(sampUV, linearDepth, projection[0][0], projection[1][1]);
	vec4 worldPosition = cameraInvView * viewPosition;
	worldPosition /= worldPosition.w;

	vec3 N = normalize(normal.xyz);
	vec3 V = normalize(position.xyz - worldPosition.xyz);
	vec3 L = vec3(0.0f);

	vec3 toLight = LightPosition.xyz - worldPosition.xyz;
	float distanceAttenuation = calcDistanceAttenuation(
		toLight, LightDirection.xyz, L, LightRadius, LightFallOffExp
	);

	vec3 lightAxisZ = normalize(LightDirection.xyz);
	vec3 lightAxisY = normalize(LightRight.xyz);
	vec3 lightAxisX = cross(lightAxisY, lightAxisZ);
	mat3 lightToWorld = mat3(lightAxisX, lightAxisY, lightAxisZ);
	float NoV = clamp(dot(N, V), 0.0f, 1.0f);
	float NoL = 0.0f;
	float baseIrradiance = 0.0f;
	vec2 lightHalfSize = vec2(LightHalfWidth, LightHalfHeight);
	vec3 averageL = lambertLightIntegrate(lightToWorld, L, N, lightHalfSize, NoL, baseIrradiance);

	vec3 F0 = mix(vec3(0.04f), albedo.rgb, metallic);
	vec3 diffuse = getLambertianParam(albedo.rgb, metallic) * LightColor.rgb * LightIntensity * distanceAttenuation * baseIrradiance * max(NoL, 0.0f);
	vec3 specular = LightColor.rgb * LightIntensity * distanceAttenuation * RectGGXApproxLTC(roughness, F0, toLight, N, V, lightHalfSize, lightAxisX, lightAxisY);

	float shadow = max(texture(shadowFactorSamp, sampUV).r, 0.05f);
//	vec3 res = (specular + diffuse) * shadow;
	vec3 res = (specular + diffuse) * 0.70f * shadow;

	DirectLight = vec4(res, 1.0f);
}