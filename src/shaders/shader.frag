#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragmentColor;
layout(location = 1) in vec2 fragmentTextureCoordinates;

layout(location = 0) out vec4 outColor;

void main() {
    // This is for debugging.
    // outColor = vec4(fragmentTextureCoordinates, 0.0, 1.0);
    // outColor = texture(textureSampler, fragmentTextureCoordinates * 3.0);
    // outColor = texture(textureSampler, fragmentTextureCoordinates);
    outColor = vec4(fragmentColor * texture(textureSampler, fragmentTextureCoordinates * 3.0).rgb, 1.0f);
}
