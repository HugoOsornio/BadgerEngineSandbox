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
 
  vec3 normalizedNormals = normalize(normal);
  vec3 lightDirection = normalize(vec3(g_ubo.lightPosition) - fragmentPosition);
  float diffuseComponent = max(dot(lightDirection, normalizedNormals), 0.0);
  vec4 diffuse = vec4(diffuseComponent, diffuseComponent, diffuseComponent, 1.0);
  
  vec3 reflectDirection = reflect(-lightDirection, normalizedNormals);
  vec3 viewDirection = normalize(-fragmentPosition);
  float specularComponent = pow(max(dot(viewDirection, reflectDirection), 0.0), 64);
  vec4 specular = vec4(specularComponent, specularComponent, specularComponent, 1.0) * 0.5;

  vec4 objectColor = vec4(0.5, 0.5, 0.5, 1.0);
  vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0);  
  
  // attenuation
  float distance = length(vec3(g_ubo.lightPosition) - fragmentPosition);
  float attenuation = 1.0 / (1.0f + 0.09 * distance + 0.032 * (distance * distance)); 
  
  ambient  *= attenuation;
  diffuse  *= attenuation;
  specular *= attenuation; 
  
  outputColor =  (ambient + diffuse + specular) * objectColor;
}