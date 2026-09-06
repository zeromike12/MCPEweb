#version 450

// Binding 1: Color Texture
layout(set = 1, binding = 0) uniform sampler2D texSampler;

// Inputs
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragUV);
    outColor = texColor * fragColor;
}
