#version 450

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragmentColor;
layout(location = 1) in vec2 fragmentTextureCoordinates;
layout(location = 2) in vec3 transformedNormal;
layout(location = 3) in vec3 lightDirection;
layout(location = 4) in vec3 viewVector;

layout(binding = 0) uniform UniformData {
    // Matrices
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;

    // Lighting
    vec4 lightAmbient;
    vec4 lightDiffuse;
    vec4 lightSpecular;
    vec4 lightPosition;

    // Material
    vec4 materialAmbient;
    vec4 materialDiffuse;
    vec4 materialSpecular;
    float materialShininess;
} ubo;

layout(location = 0) out vec4 outColor;

vec4 computePhongAdsLight() {
    vec3 normalizedTransformedNormal = normalize(transformedNormal);
    vec3 normalizedLightDirection = normalize(lightDirection);
    vec3 normalizedViewVector = normalize(viewVector);
    vec3 reflectionVector = reflect(-normalizedLightDirection, normalizedTransformedNormal);
    float transformedNormalDotLightDirection = max(dot(normalizedLightDirection, normalizedTransformedNormal), 0.0);
    float specularFactor = pow(max(dot(reflectionVector, normalizedViewVector), 0.0), ubo.materialShininess);

    vec3 ambientLight = vec3(ubo.lightAmbient) * vec3(ubo.materialAmbient);
    vec3 diffuseLight = vec3(ubo.lightDiffuse) * vec3(ubo.materialDiffuse) * transformedNormalDotLightDirection;
    vec3 specularLight = vec3(ubo.lightSpecular) * vec3(ubo.materialSpecular) * specularFactor;

    return vec4(ambientLight + diffuseLight + specularLight, 1.0);
}

void main()
{
    vec4 phongAdsLight = computePhongAdsLight();
    vec3 normalizedTransformedNormal = normalize(transformedNormal);
    outColor = texture(textureSampler, fragmentTextureCoordinates) * phongAdsLight;
    // outColor = phongAdsLight;
    // outColor = ubo.materialAmbient;
    // outColor = ubo.materialDiffuse;
    // outColor = ubo.materialSpecular;
    // outColor = vec4(ubo.materialShininess, ubo.materialShininess, ubo.materialShininess, 1.0);
    // outColor = ubo.lightAmbient;
    // outColor = ubo.lightDiffuse;
    // outColor = ubo.lightSpecular;
    // outColor = ubo.lightPosition;
    // outColor = texture(textureSampler, fragmentTextureCoordinates);
}
