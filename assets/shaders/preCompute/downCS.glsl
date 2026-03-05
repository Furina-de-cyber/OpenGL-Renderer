#version 460 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(rgba8, binding = 0) uniform readonly imageCube origin;
layout(rgba8, binding = 1) uniform writeonly imageCube diffuse;
layout(rgba8, binding = 2) uniform writeonly imageCube specular;

uniform int inWidth;
uniform int inHeight;

void main()
{
    ivec3 coord = ivec3(gl_GlobalInvocationID.xyz);
    if (coord.x >= inWidth || coord.y >= inHeight || coord.z >= 6){
        return;
    }

}