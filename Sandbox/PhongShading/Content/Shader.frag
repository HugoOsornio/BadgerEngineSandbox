#version 450

layout(location = 0) in vec3 fragmentPosition;
layout(location = 1) in vec3 normal;

layout(std140, binding = 1) uniform UBO
{
  vec4 lightPosition;
  vec4 lightIntensity;
}
g_ubo;

layout(location = 0) out vec4 outputColor;

void main()
{
  vec3 s = normalize(vec3(g_ubo.lightPosition) - fragmentPosition);
  vec4 Kd = vec4(0.8, 0.8, 0.8, 1.0);
  vec4 Ka = vec4(0.1, 0.1, 0.1, 1.0);
  outputColor =  g_ubo.lightIntensity * (Ka + (Kd*max(dot(s, normal), 0.0f)));
}