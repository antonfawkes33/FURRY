#version 450

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 projection;
    float progress; // 0.0 to 1.0 (0=fully visible, 1=fully dissolved)
} pc;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
    float noise = rand(fragTexCoord);
    if (noise < pc.progress) {
        discard;
    }
    outColor = texture(texSampler, fragTexCoord) * fragColor;
}
