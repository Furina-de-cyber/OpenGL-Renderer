#version 460 core

layout(location = 0) out vec4 envBRDF;

uniform int sampleNums;
uniform float size;   // width = height = size

const float PI = 3.14159265f;

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

float D_GGX( float a2, float NoH )
{
	float d = ( NoH * a2 - NoH ) * NoH + 1;
	return a2 / ( PI*d*d );
}

float G_Smith(float roughness, float NoL, float NoV){
//    float k = pow(roughness + 1, 2) / 8;
    float k = pow(roughness, 2) / 2;
    float masking   = NoL / (NoL * (1 - k) + k);
    float shadowing = NoV / (NoV * (1 - k) + k);
    return masking * shadowing;
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
    vec2 uv = (gl_FragCoord.xy + 0.5) / vec2(size);
    float NoV       = clamp(uv.x, 0.0, 1.0);
    float roughness = clamp(uv.y, 0.0, 1.0);
    float alpha = roughness * roughness;
    NoV = clamp(NoV, 0.0f, 1.0f);
    vec3 V = vec3(sqrt(1 - NoV * NoV), 0, NoV);
    vec3 N = vec3(0.0f, 0.0f, 1.0f);
	float A = 0.0f, B = 0.0f;
    for(int i=0 ;i<sampleNums;i++){
//        vec2 Xi = Hammersley(i, sampleNums);
//        vec2 scramble = hash22(gl_FragCoord.xy);
//        vec2 random = fract(Xi + scramble);
        vec2 random = Hammersley(i, sampleNums);
        vec3 H = normalize(importanceSampleGGX(random, alpha));
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float VoH = clamp(dot(V, H), 0.0f, 1.0f);
        float NoH = clamp(dot(N, H), 0.0f, 1.0f);
        float NoL = dot(N, L);
        
        if(NoL > 0){
            float Fc = pow(1.0f - VoH, 5.0f);
            float core = G_Smith(roughness, NoL, NoV) * VoH / (NoV * NoH); 
            A += (1 - Fc) * core;
            B += Fc * core;
        }
    }
    envBRDF = vec4(vec2(A, B)/float(sampleNums), 0.0f, 1.0f);
}