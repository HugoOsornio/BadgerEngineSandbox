#version 450

layout(location = 0) in vec3 fragmentPosition;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 fragmentPositionLightSpace;

layout(std140, binding = 1) uniform UBO
{
  vec4 lightPosition;
  vec4 lightIntensity;
}
g_ubo;

layout(binding = 2) uniform sampler2D shadowMap;

layout(location = 0) out vec4 outputColor;

vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0);

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.1;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(shadowMap, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
}

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
  
  // attenuation
  float distance = length(vec3(g_ubo.lightPosition) - fragmentPosition);
  float attenuation = 1.0 / (1.0f + 0.09 * distance + 0.032 * (distance * distance)); 
  
  ambient  *= attenuation;
  diffuse  *= attenuation;
  specular *= attenuation;
  
  float shadow = filterPCF(fragmentPositionLightSpace / fragmentPositionLightSpace.w);
  
  outputColor =  (shadow * (diffuse + specular)) * objectColor;
}