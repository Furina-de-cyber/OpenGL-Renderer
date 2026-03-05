#version 460 core

layout(location = 0) out float Shadow_Map;

in float lightDepth;
in float lightBias;
uniform vec2 lightFN; // x is far, y is near

void main()
{
//    float shadowDepth = 1 - lightDepth;
//    shadowDepth = shadowDepth + lightBias;
    float far = lightFN.x;
    float near = lightFN.y;
    float z_ndc = gl_FragCoord.z * 2.0f - 1.0f;
    float z_view = 2*near*far/(near+far-(far-near)*z_ndc);
    float shadowDepth = (z_view-near)/(far-near);
    shadowDepth = shadowDepth + lightBias;
    Shadow_Map = clamp(shadowDepth, 0.0f, 1.0f);
}
