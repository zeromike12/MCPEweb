#version 450

// Binding 0: Frame Constants
layout(set = 0, binding = 0) uniform FrameConstants {
    mat4 mvp;
    mat4 view;
    vec4 fogColor;
    float fogStart;
    float fogEnd;
    float gameTime;
    float _pad;
} u_frame;

// Attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inLightmapUV;

// Outputs
layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

void main() {
    // For sky and HUD we generally don't apply fog
    gl_Position = u_frame.mvp * vec4(inPosition, 1.0);
    fragUV = inUV;
    fragColor = inColor;
}
