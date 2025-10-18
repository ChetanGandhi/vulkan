#version 450

layout(binding = 0) uniform uniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;

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

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTextureCoordinates;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragmentColor;
layout(location = 1) out vec2 fragmentTextureCoordinates;
layout(location = 2) out vec3 transformedNormal;
layout(location = 3) out vec3 lightDirection;
layout(location = 4) out vec3 viewVector;

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model * inPosition;

    vec4 eyeCoordinates = ubo.view * ubo.model  * inPosition;
    mat3 normalMatrix = mat3(transpose(inverse(ubo.view * ubo.model)));
    transformedNormal = normalMatrix * inNormal;
    lightDirection = vec3(ubo.lightPosition - eyeCoordinates);
    viewVector = -eyeCoordinates.xyz;
    fragmentColor = inColor;
    fragmentTextureCoordinates = inTextureCoordinates;
}
