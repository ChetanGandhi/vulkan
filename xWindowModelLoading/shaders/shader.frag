#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragmentColor;
layout(location = 1) in vec2 fragmentTextureCoordinates;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textureSampler, fragmentTextureCoordinates);
}
