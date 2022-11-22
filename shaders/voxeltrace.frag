#version 450

struct Cell {
    uint data;
};

struct Grid {
    Cell val;
    Cell cells[8];
};



layout(binding = 1) uniform VoxelVolume{
    vec4 transform; //Placement it in the real world
    Grid grids[];
} pool;


layout(binding = 2) uniform sampler2D image; //Can be used for voxel textures

layout(location = 0) in vec3 pos;

layout(location = 0) out vec4 outColor;

bool traceNode(vec3 orig, vec3 dir, inout float t, vec3 nodeMin, float size){
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
/*
void traceIntoVolume(vec3 orig, vec3 dir, Grid grid, uint depth){
    traceNode(orig, dir, t, nodeMin, size);
}*/

void main() {
    vec3 orig = vec3(0.0, 0.0, 1.0);
    vec3 dir = normalize(pos - orig); //Ray direction shooting from orig to a plane at z=0 (x,y)=[-1,1]x[-1,1]
    float t;
    vec3 nodeMin = pool.transform.xyz;
    float size = pool.transform.w;
    //traceIntoVolume(orig, dir, pool.grids[0], 0);

    if (traceNode(orig, dir, t, nodeMin, size)){
        outColor.xyz = vec3(1.0,0.0,0.0);
    }
    else{
        outColor.xyz = vec3(0.0,0.0,1.0);
    }
    uint vertexColor = pool.grids[0].val.data;
    vec4 color = vec4((vertexColor & 0x000000FF) >> 0, (vertexColor & 0x0000FF00) >> 8, (vertexColor & 0x00FF0000) >> 16, (vertexColor & 0xFF000000)>> 24);
    outColor.xyz = color.xyz;
    outColor.w = 1.0;

}
