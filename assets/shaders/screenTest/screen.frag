#version 460 core

in vec2 uv;
uniform sampler2D samp;
const float exposure = 1.0f;

out vec4 FragColor;

vec3 toneMappingACES(vec3 x) {
    x *= exposure;
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
vec3 toneMappingReinhard(vec3 x) {
    x *= exposure;
    return x / (1.0 + x);
}
vec3 gammaCorrect(vec3 color) {
    return mix(color * 12.92, 1.055 * pow(color, vec3(1.0/2.4)) - 0.055, step(0.0031308, color));
}
vec3 gammaCorrectSimple(vec3 color) {
    return pow(abs(color), vec3(0.454546f));
}
void main() {
	FragColor = vec4(texture(samp, uv).rgb, 1.0f);
//    vec3 colorLinear = texture(samp, uv).rgb;
//    vec3 colorUnlinear = toneMappingACES(colorLinear);
//    FragColor = vec4(colorUnlinear, 1.0f);
}