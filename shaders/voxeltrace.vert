#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 ONB;
    vec4 eyeDist;
    float ratio;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

//layout(location = 0) out vec3 fragColor;
layout(location = 0) out vec3 dir;
void main() {
    gl_Position =  vec4(inPosition, 0.0, 1.0);
    dir = vec3(ubo.ONB*vec4(inTexCoord.x*max(ubo.ratio,1),inTexCoord.y*max(1/ubo.ratio,1),0.0,1.0));//Ray direction shooting from orig to a plane at z=0 (x,y)=[-1,1]x[-1,1]
    dir.z = -ubo.eyeDist.w;
}
