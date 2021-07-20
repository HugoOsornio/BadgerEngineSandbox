#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(std140, binding = 0) uniform UBO
{
  mat4 modelView;
  mat4 MVP;
}
g_mats;

out gl_PerVertex
{
  vec4 gl_Position;
};

layout(location = 0) out vec4 vertexColor;

void main() 
{
    vertexColor = inColor;
	gl_Position = g_mats.MVP * inPosition;
}