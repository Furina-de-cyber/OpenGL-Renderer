#version 460 core

layout(location = 0) out vec4 diffuse;
layout(location = 1) out vec4 specularR1;
layout(location = 2) out vec4 specularR2;
layout(location = 3) out vec4 specularR3;
layout(location = 4) out vec4 specularR4;
layout(location = 5) out vec4 specularR5;

uniform int faceIndex; // +x, -x, +y, -y, +z, -z
uniform int sampleNumsDiffuse;
uniform int sampleNumsSpecular;
uniform float size;   // width = height = size
uniform samplerCube cubemapSampler;

const float PI = 3.14159265f;
const float roughness[5] = {0.00f, 0.25f, 0.50f, 0.75f, 1.00f};

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 2^32
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(
        float(i) / float(N),
        RadicalInverse_VdC(i)
    );
}

vec3 importanceSampleGGX(vec2 random, float roughness2){
    float u1 = random.x;
    float u2 = random.y;

    float r = u1 < 1.0f ? roughness2 * sqrt(u1 / (1 - u1)) : 0.0f;
    float cosTheta = u1 < 1.0f ? 1 / sqrt(1 + r * r) : 0.0f;
    cosTheta = clamp(cosTheta, 0.0f, 1.0f);
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
    float phi = 2 * PI * u2;

    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = cosTheta;
    return vec3(x, y, z);
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

float D_GGX( float a2, float NoH )
{
	float d = ( NoH * a2 - NoH ) * NoH + 1;
	return a2 / ( PI*d*d );
}

vec2 hash22(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float ComputeCubemapMipFromRoughness(
    float Roughness,
    float MipCount,
    float BaseResolution
)
{
    float logRes = log2(BaseResolution);
    float Level = (logRes - 6) - 1.15 * log2(Roughness);
    return MipCount - 1 - Level;
}

void main(){
	vec2 uv = (gl_FragCoord.xy + 0.5f) / size;
    uv = uv * 2.0f - 1.0f;
    
    vec3 direction = vec3(0.0f);   //    z
    vec3 right     = vec3(0.0f);
    switch(faceIndex) {  
        case 0: // +X
            direction = vec3(1.0f, -uv.y, -uv.x);
            right     = vec3(0.0f, 1.0f, 0.0f);
            break;
        case 1: // -X
            direction = vec3(-1.0f, -uv.y, uv.x);
            right     = vec3(0.0f, 1.0f, 0.0f);
            break;
        case 2: // +Y
            direction = vec3(uv.x, 1.0f, uv.y);
            right     = vec3(0.0f, 0.0f, 1.0f);
            break;
        case 3: // -Y
            direction = vec3(uv.x, -1.0f, -uv.y);
            right     = vec3(0.0f, 0.0f, 1.0f);
            break;
        case 4: // +Z
            direction = vec3(uv.x, -uv.y, 1.0f);
            right     = vec3(1.0f, 0.0f, 0.0f);
            break;
        case 5: // -Z
            direction = vec3(-uv.x, -uv.y, -1.0f);
            right     = vec3(1.0f, 0.0f, 0.0f);
            break;
    }
    vec3 axisZ = normalize(direction);
    vec3 axisY = normalize(cross(axisZ, right));
    vec3 axisX = normalize(cross(axisY, axisZ));
    mat3 localToWorld = mat3(axisX, axisY, axisZ);

    vec3 sum = vec3(0.0f);
    for(int i = 0; i < sampleNumsDiffuse; i++){
        vec2 Xi = Hammersley(i, sampleNumsDiffuse);
        vec2 scramble = hash22(gl_FragCoord.xy + float(faceIndex) * 31.0);
        vec2 random = fract(Xi + scramble);
        vec3 localVector = importanceSampleCosineWeighted(random);
        vec3 sampleVector = localToWorld * localVector;
        vec4 color = textureLod(cubemapSampler, sampleVector, 0.0f);
        sum = sum + color.rgb;
    }
    // cosine weighted sampling
    // mento carlo  \frac{1}{N}*\sum_{i}^{N}\frac{L_i * cos\theta}{\frac{cos\theta}{\pi}}
    // RE without albedo : baseColor(1-metallic)/pi
    vec3 averageColor = PI * sum / float(sampleNumsDiffuse);
    diffuse = vec4(averageColor/PI, 1.0f);

    vec3 specularSum[5] = {vec3(0.0f),vec3(0.0f),vec3(0.0f),vec3(0.0f),vec3(0.0f)};
    float weight[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    for(int i = 0; i < sampleNumsSpecular; i++){
        vec2 Xi = Hammersley(i, sampleNumsSpecular);
        vec2 scramble = hash22(gl_FragCoord.xy + float(faceIndex) * 31.0);
        vec2 random = fract(Xi + scramble);
        for(int j = 0; j < 5; j++){
            float alpha = roughness[j]*roughness[j];
            vec3 V = vec3(0.0f, 0.0f, 1.0f); // N
            vec3 N = V;
            vec3 H = normalize(importanceSampleGGX(random, alpha));
            vec3 L  = normalize(2.0 * dot(V, H) * H - V);
            float NoL = clamp(dot(N, L), 0.0f, 1.0f);
            vec3 sampleVector = localToWorld * L;
            vec4 color = textureLod(cubemapSampler, sampleVector, ComputeCubemapMipFromRoughness(roughness[j], 11, 1024));
            specularSum[j] = specularSum[j] + color.rgb * NoL;
            weight[j] = weight[j] + NoL;
        }
    }
    vec3 averageSpecular[5] = {vec3(0.0f),vec3(0.0f),vec3(0.0f),vec3(0.0f),vec3(0.0f)};
    for(int i=0;i<5;i++){
        averageSpecular[i] = specularSum[i] / weight[i];
    }
    specularR1 = vec4(averageSpecular[0], 1.0f);
    specularR2 = vec4(averageSpecular[1], 1.0f);
    specularR3 = vec4(averageSpecular[2], 1.0f);
    specularR4 = vec4(averageSpecular[3], 1.0f);
    specularR5 = vec4(averageSpecular[4], 1.0f);
}