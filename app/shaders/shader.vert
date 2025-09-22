#version 450

layout(binding = 0) uniform uniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTextureCoordinates;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragmentColor;
layout(location = 1) out vec2 fragmentTextureCoordinates;
layout(location = 2) out vec3 transformedNormal;
layout(location = 4) out vec3 transformedTangent;

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);

    mat3 normalMatrix = mat3(transpose(inverse(ubo.model)));
    transformedNormal = normalize(normalMatrix * inNormal);
    fragmentColor = inNormal;
    fragmentTextureCoordinates = inTextureCoordinates;
    transformedTangent = normalize(normalMatrix * inTangent);
}
