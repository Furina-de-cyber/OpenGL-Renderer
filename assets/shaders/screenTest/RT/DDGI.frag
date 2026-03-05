#version 460 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : require

layout(std140) uniform camera
{
	mat4 projection;
	mat4 view;
	vec4 position;
};

struct FProbe {
    vec3 mPosition;
    uint mSampleNums;
};

layout(location = 0) out vec4 DDGI;

layout(std430, binding = 0) buffer irradianceHandles{
	uint64_t irradiance[];
};
layout(std430, binding = 1) buffer depthHandles{
	uint64_t depthAndDepthSquare[];
};
layout(std430, binding = 6) buffer probeBuffer{
	FProbe probes[];
};

uniform sampler2D AlbedoSamp;
uniform sampler2D RoughnessMetallicLinearDepthSamp;
uniform sampler2D NormalSamp;

uniform mat4 cameraInvView;
uniform vec3 probeMatrixSize;
uniform vec3 probeMatrixCenter;
uniform vec3 probeGridSize;
uniform int frameIndexMod8;

in vec2 uv;

const float PI = 3.14159265f;

float hash12(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 78.233);
    return fract(p.x * p.y);
}

float rand(vec2 uv, float frame) {
    return hash12(uv + frame);
}

vec3 sampleSphere(vec2 uv, float frame) {
    float u1 = rand(uv, frame);
    float u2 = rand(uv + 0.5, frame);
    float u3 = rand(uv + 1.0, frame);

    float theta = 2.0 * 3.14159265359 * u1;
    float phi = acos(1.0 - 2.0 * u2);

    float r = pow(u3, 1.0 / 3.0);

    vec3 dir;
    dir.x = sin(phi) * cos(theta);
    dir.y = sin(phi) * sin(theta);
    dir.z = cos(phi);

    return r * dir;
}

vec4 calcViewPosition(vec2 sampUV, float zview, float proj00, float proj11){
	vec2 clipXY = sampUV * 2.0f - 1.0f;
	return vec4(clipXY  * ( - zview) / vec2(proj00, proj11), zview, 1.0f);
}

vec2 OctEncode(vec3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));

    if (n.z < 0.0)
        n.xy = (1.0 - abs(n.yx)) * sign(n.xy);

    return n.xy * 0.5 + 0.5;
}

int probePosition2Index(ivec3 gridPosition, ivec3 gridSize)
{
    return gridPosition.x * gridSize.y * gridSize.z
         + gridPosition.y * gridSize.z
         + gridPosition.z;
}

void findDDGIProbes4x4x4(
    vec3 fragPosition,
    vec3 probeGridMin,
    vec3 probeSpacing,
    out ivec4 index1,
    out ivec4 index2,
    out vec4  weight1,
    out vec4  weight2)
{
    const ivec3 PROBE_GRID_SIZE = ivec3(4);
    vec3 probeCoord = (fragPosition - probeGridMin) / probeSpacing;
    ivec3 base = ivec3(floor(probeCoord));
    ivec3 maxBase = PROBE_GRID_SIZE - ivec3(2);
    base = clamp(base, ivec3(0), maxBase);
    vec3 frac = clamp(probeCoord - vec3(base), vec3(0.0), vec3(1.0));

    index1.x = probePosition2Index(base + ivec3(0,0,0), PROBE_GRID_SIZE);
    index1.y = probePosition2Index(base + ivec3(0,0,1), PROBE_GRID_SIZE);
    index1.z = probePosition2Index(base + ivec3(0,1,0), PROBE_GRID_SIZE);
    index1.w = probePosition2Index(base + ivec3(0,1,1), PROBE_GRID_SIZE);

    index2.x = probePosition2Index(base + ivec3(1,0,0), PROBE_GRID_SIZE);
    index2.y = probePosition2Index(base + ivec3(1,0,1), PROBE_GRID_SIZE);
    index2.z = probePosition2Index(base + ivec3(1,1,0), PROBE_GRID_SIZE);
    index2.w = probePosition2Index(base + ivec3(1,1,1), PROBE_GRID_SIZE);

    weight1.x = (1.0 - frac.x) * (1.0 - frac.y) * (1.0 - frac.z); // 000
    weight1.y = (1.0 - frac.x) * (1.0 - frac.y) * (      frac.z); // 001
    weight1.z = (1.0 - frac.x) * (      frac.y) * (1.0 - frac.z); // 010
    weight1.w = (1.0 - frac.x) * (      frac.y) * (      frac.z); // 011

    weight2.x = (      frac.x) * (1.0 - frac.y) * (1.0 - frac.z); // 100
    weight2.y = (      frac.x) * (1.0 - frac.y) * (      frac.z); // 101
    weight2.z = (      frac.x) * (      frac.y) * (1.0 - frac.z); // 110
    weight2.w = (      frac.x) * (      frac.y) * (      frac.z); // 111
}

vec3 fetchProbeInfo(vec3 worldPosition, vec3 normal, int probeIndex){
    vec3 probePosition = probes[probeIndex].mPosition;
    vec3 frag2probe = probePosition - worldPosition;
    float t = length(frag2probe);
    frag2probe /= t;
    vec3 irradianceInfo = textureLod(sampler2D(irradiance[probeIndex]), OctEncode(frag2probe), 0.0f).rgb; 
    vec2 depthAndDepthSquareInfo = textureLod(sampler2D(depthAndDepthSquare[probeIndex]), OctEncode(frag2probe), 0.0f).rg; 
    float mean = depthAndDepthSquareInfo.x;
    float var = max(depthAndDepthSquareInfo.y - mean * mean, 0.00001f);
    float ChebyshevWeight = t > mean ? var / (var + pow(t - mean, 2.0f))  : 1.0f;
    float visibility = clamp(ChebyshevWeight, 0.0f, 1.0f);
    visibility = smoothstep(0.0f, 1.0f, visibility);
    float NoL = clamp(dot(normal, frag2probe), 0.0f, 1.0f);
    return irradianceInfo * visibility * NoL;
}

vec3 EvaluateDDGIDiffuse(
    vec3 worldPos,
    vec3 normal,
    vec3 albedo,
    ivec4 idx1,
    ivec4 idx2,
    vec4  w1,
    vec4  w2)
{
    vec3 irradiance = vec3(0.0);
    float totalW = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        vec3 c1 = fetchProbeInfo(worldPos, normal, idx1[i]);
        irradiance += c1 * w1[i];
        totalW += w1[i];

        vec3 c2 = fetchProbeInfo(worldPos, normal, idx2[i]);
        irradiance += c2 * w2[i];
        totalW += w2[i];
    }

    irradiance /= max(totalW, 1e-4);

    return irradiance * albedo * (1.0 / PI);
}

vec3 fetchProbeInfoSmooth(vec3 worldPos, vec3 normal, int probeIndex)
{
    vec3 probePos = probes[probeIndex].mPosition;
    vec3 sphereBias = sampleSphere(uv, frameIndexMod8) * 0.2f;
    probePos += sphereBias;
    vec3 dir = probePos - worldPos;
    float dist = length(dir);
    dir /= dist;

    vec2 uvS = OctEncode(dir);

    float texSize = 64.0;
    vec2 texelSize = 1.0 / vec2(texSize);

    vec3 c00 = textureLod(sampler2D(irradiance[probeIndex]), uvS, 0.0).rgb;
    vec3 c10 = textureLod(sampler2D(irradiance[probeIndex]), uvS + vec2(texelSize.x, 0.0), 0.0).rgb;
    vec3 c01 = textureLod(sampler2D(irradiance[probeIndex]), uvS + vec2(0.0, texelSize.y), 0.0).rgb;
    vec3 c11 = textureLod(sampler2D(irradiance[probeIndex]), uvS + texelSize, 0.0).rgb;

    vec3 probeIrradiance = mix(mix(c00, c10, fract(uvS.x*texSize)),
                               mix(c01, c11, fract(uvS.x*texSize)),
                               fract(uvS.y*texSize));

    vec2 depthMom = textureLod(sampler2D(depthAndDepthSquare[probeIndex]), uvS, 0.0).rg;
    float mean = depthMom.x;
    float var  = max(depthMom.y - mean*mean, 0.0001);
    float Chebyshev = dist > mean ? var / (var + pow(dist - mean, 2.0)) : 1.0;
    float visibility = clamp(Chebyshev, 0.0, 1.0);
    visibility = smoothstep(0.0, 1.0, visibility);

    float NoL = clamp(dot(normal, dir), 0.0, 1.0);

//    return probeIrradiance * visibility * NoL;
    return probeIrradiance * visibility;
}

vec3 EvaluateDDGIDiffuseSmooth(
    vec3 worldPos,
    vec3 normal,
    vec3 albedo,
    ivec4 idx1,
    ivec4 idx2,
    vec4  w1,
    vec4  w2)
{
    vec4 sw1 = w1*w1*(3.0 - 2.0*w1);
    vec4 sw2 = w2*w2*(3.0 - 2.0*w2);

    vec3 irradiance = vec3(0.0);
    float totalW = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        vec3 c1 = fetchProbeInfoSmooth(worldPos, normal, idx1[i]);
        vec3 c2 = fetchProbeInfoSmooth(worldPos, normal, idx2[i]);
        irradiance += c1 * sw1[i] + c2 * sw2[i];
        totalW += sw1[i] + sw2[i];
    }

    irradiance /= max(totalW, 1e-4);
    irradiance *= albedo * (1.0/PI);

    return irradiance;
}


void main() {

	vec2 sampUV = uv;
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
    vec3 probeGridMin = probeMatrixCenter - probeMatrixSize * 0.5f;
    vec3 probeSpacing = probeMatrixSize / (4.0f - 1.0f);
    ivec4 index1  = ivec4(0);
    ivec4 index2  = ivec4(0);
    vec4  weight1 = vec4(0.0f);
    vec4  weight2 = vec4(0.0f);
    findDDGIProbes4x4x4(worldPosition.xyz, probeGridMin, probeSpacing, index1, index2, weight1, weight2);
//    vec3 outputColor = EvaluateDDGIDiffuse(worldPosition.xyz, N, albedo.rgb, index1, index2, weight1, weight2);
    vec3 outputColor = EvaluateDDGIDiffuseSmooth(worldPosition.xyz, N, albedo.rgb, index1, index2, weight1, weight2);
    
    DDGI = vec4(outputColor, 1.0f);
//    DDGI = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}