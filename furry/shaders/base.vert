#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(push_constant) uniform Push {
    mat4 projection;
} push;

void main() {
    gl_Position = push.projection * vec4(inPos, 0.0, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
