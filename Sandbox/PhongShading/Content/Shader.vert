#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(std140, binding = 0) uniform UBO
{
  mat4 normal;
  mat4 modelView;
  mat4 MVP;
}
g_mats;

out gl_PerVertex
{
  vec4 gl_Position;
};

layout(location = 0) out vec3 fragmentPosition;
layout(location = 1) out vec3 normal;

void main() 
{
	normal =  normalize(mat3(g_mats.normal) * inNormal);
    fragmentPosition = vec3(g_mats.modelView * vec4(inPosition, 1.0));
	gl_Position = g_mats.MVP * vec4(inPosition, 1.0);
}