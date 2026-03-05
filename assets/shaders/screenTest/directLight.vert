#version 460 core

layout(location = 0) in vec3 aPos;

uniform mat4 worldToClip; // projection * view * model
out vec4 clipPosition;

// out vec4 clipPosition is original position in clipPosition space
// gl_Position is the final position after perspective devision
void main() {
	clipPosition = worldToClip * vec4(aPos, 1.0f); 
	gl_Position = clipPosition;
}