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

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normalizedNormals = normalize(normal);
    vec3 lightDir = normalize(vec3(g_ubo.lightPosition) - fragmentPosition);
    float bias = max(0.05 * (1.0 - dot(normalizedNormals, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
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
  vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0);  
  
  // attenuation
  float distance = length(vec3(g_ubo.lightPosition) - fragmentPosition);
  float attenuation = 1.0 / (1.0f + 0.09 * distance + 0.032 * (distance * distance)); 
  
  ambient  *= attenuation;
  diffuse  *= attenuation;
  specular *= attenuation;
  
  float shadow = ShadowCalculation(fragmentPositionLightSpace);
  
  outputColor =  (ambient + (1 - shadow) * (diffuse + specular)) * objectColor;
}