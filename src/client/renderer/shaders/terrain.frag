#version 450

// Binding 0: Frame Constants (UBO) - already bound in vertex, but accessible if needed
layout(set = 0, binding = 0) uniform FrameConstants {
    mat4 mvp;
    mat4 view;
    vec4 fogColor;
    float fogStart;
    float fogEnd;
    float gameTime;
    float _pad;
} u_frame;

// Binding 1: Atlas Texture
layout(set = 1, binding = 0) uniform sampler2D texSampler;

// Inputs from Vertex Shader
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in float fragFogFactor;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragUV);
    
    // Alpha Test (Minecraft uses Alpha-To-Coverage or direct clip)
    if (texColor.a < 0.1) {
        discard;
    }

    vec4 finalColor = texColor * fragColor;
    
    // Apply Fog
    outColor = mix(u_frame.fogColor, finalColor, fragFogFactor);
}
