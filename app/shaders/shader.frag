#version 450

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragmentColor;
layout(location = 1) in vec2 fragmentTextureCoordinates;
layout(location = 2) in vec3 transformedNormal;
layout(location = 4) in vec3 transformedTangent;

layout(location = 0) out vec4 outColor;

vec2 FurMaskMapping(vec2 texCoords, vec3 viewDir, int numLayers, float heightScale) {
    float layerDepth = 1.0 / float(numLayers);

    vec2 P = viewDir.xy * heightScale;
    vec2 deltaTexCoords = P / float(numLayers);

    vec2 currentTexCoords = texCoords;

    for (int i = 0; i < numLayers; i++) {
        float mask = texture(textureSampler, currentTexCoords).a;

        if (mask > 0.5) {
            return currentTexCoords; // fur hit
        }

        currentTexCoords -= deltaTexCoords;
    }

    return texCoords; // fallback (no fur hit)
}

void main()
{
    vec3 normalizedTransformedNormal = normalize(transformedNormal);
    vec3 normalizedTransformedTangent = normalize(transformedTangent);

    vec3 biTangent = cross(normalizedTransformedNormal, normalizedTransformedTangent );
    mat3 tbn = mat3(transformedTangent, biTangent, normalizedTransformedNormal);

    vec3 viewDirection = transpose(tbn) * vec3(0.0,0.0,1.0);
    vec2 distortedTexcoord = FurMaskMapping(fragmentTextureCoordinates, viewDirection, 8, 10.0);
    vec4 tex = texture(textureSampler, distortedTexcoord);
    // outColor.rgb = dot(normalizedTransformedNormal, vec3(1.0)) * tex.rgb;
    // outColor.rgb = dot(vec3(1.0), vec3(1.0)) * tex.rgb;
    // outColor.rgb = fragmentColor;
    outColor.rgb = tex.rgb;
    outColor.rgb = normalizedTransformedTangent;
}
