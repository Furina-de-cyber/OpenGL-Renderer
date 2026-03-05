#version 460 core

layout(std140) uniform camera
{
    mat4 projection;
    mat4 view;
    vec4 position;
};

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUv;
layout(location = 2) in vec3 aNormal;

uniform mat4 modelMatrix;
uniform mat3 normalMatrix;

out vec2 uv;
out vec3 normal;
out vec4 clipPosition;

void main()
{
    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);
    clipPosition = projection * view * worldPos;
    gl_Position = clipPosition;
    normal = normalize(normalMatrix * aNormal);
    uv = aUv;
}
