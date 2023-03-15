#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 ONB;
  vec4 eyeDist;
  float ratio;
  uint framenum;

}
ubo;

// Gold Noise function
float PHI = 1.61803398874989484820459 * 00000.1; // Golden Ratio   
float PI  = 3.14159265358979323846264 * 00000.1; // PI
float SRT = 1.41421356237309504880169 * 10000.0; // Square Root of Two


float random_0t1(in vec2 coordinate, in float seed)
{
    return fract(sin(dot(coordinate*seed, vec2(PHI, PI)))*SRT);
}

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

// layout(location = 0) out vec3 fragColor;
layout(location = 0) out vec3 dir;
void main() {
  gl_Position = vec4(inPosition, 0.0, 1.0);
  dir = normalize(
      vec3(ubo.ONB * vec4((inTexCoord.x+(random_0t1(inTexCoord,float(ubo.framenum*2))*2.-1.)/1920.0)   * max(ubo.ratio, 1),
                          (inTexCoord.y+(random_0t1(inTexCoord,float(ubo.framenum*2+1))*2.-1.)/1080.0) * max(1 / ubo.ratio, 1), 
                          -ubo.eyeDist.w,
                          1.0))); // Ray direction shooting from orig to a plane
                                  // at z=0 (x,y)=[-1,1]x[-1,1]
}
