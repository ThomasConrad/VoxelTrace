#version 450

layout(binding = 1) uniform sampler2D pool;

layout(location = 0) in vec3 pos;

layout(location = 0) out vec4 outColor;

bool traceNode(vec3 orig, vec3 dir, inout float t, vec3 nodeMin, int size){
    vec3 invdir = 1.0/dir;
    vec3 t0 = (nodeMin-orig)*invdir;
    vec3 nodeMax = nodeMin-vec3(size);
    vec3 t1 = (nodeMax-orig)*invdir;

    vec3 tmin = min(t0,t1);
    vec3 tmax = max(t0,t1);
    
    t = max(tmin.x, max(tmin.y, tmin.z));
    float tmax_ = min(tmax.x, min(tmax.y, tmax.z));
    
    return (t < tmax_ && t > 0);
}

void main() {
    vec3 orig = vec3(0.0, 0.0, 1.0);
    vec3 dir = normalize(pos - orig); //Ray direction shooting from orig to a plane at z=0 (x,y)=[-1,1]x[-1,1]
    float t;
    vec3 nodeMin = vec3(0.0, 0.0, -3.0);
    int size = 1;
    if (traceNode(orig, dir, t, nodeMin, size)){
        outColor.xyz = vec3(1.0,0.0,0.0);
    }
    else{
        outColor.xyz = vec3(0.0,0.0,1.0);
    }
    outColor.w = 1.0;

}
