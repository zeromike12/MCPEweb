#version 450

// Binding 0: Frame buffer (UBO)
layout(set = 0, binding = 0) uniform FrameConstants {
    mat4 mvp;
    mat4 view;
    vec4 fogColor;
    float fogStart;
    float fogEnd;
    float gameTime;
    float _pad;
} u_frame;

// Vertex Attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inLightmapUV;

// Outputs to Fragment Shader
layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float fragFogFactor;

void main() {
    vec4 pos = vec4(inPosition, 1.0);
    gl_Position = u_frame.mvp * pos;

    fragUV = inUV;
    fragColor = inColor;

    // Linear Fog
    vec4 viewPos = u_frame.view * pos;
    float dist = length(viewPos.xyz);
    fragFogFactor = clamp((u_frame.fogEnd - dist) / (u_frame.fogEnd - u_frame.fogStart), 0.0, 1.0);
}
