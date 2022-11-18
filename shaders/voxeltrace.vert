#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

//layout(location = 0) out vec3 fragColor;
layout(location = 0) out vec3 pos;

void main() {
    gl_Position =  ubo.proj * vec4(inPosition, 0.0, 1.0);
    pos = vec3(ubo.model * ubo.view * ubo.proj * (vec4(inTexCoord,0.0,1.0)*2-1));
}
