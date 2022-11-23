#version 450

struct Cell {
    uint data;
};

struct Grid {
    Cell val;
    Cell cells[8];
};

struct BBox {
    vec3 corner;
    float size;
};

layout(binding = 0) uniform UniformBufferObject {
    mat3 ONB;
    vec4 eyeDist;
    float ratio;
} ubo;

layout(binding = 1) uniform VoxelVolume{
    vec4 transform; //Placement it in the real world
    Grid grids[];
} pool;

layout(binding = 2) uniform sampler2D image; //Can be used for voxel textures :)

layout(location = 0) in vec3 dir;

layout(location = 0) out vec4 outColor;

bool traceNode(vec3 orig, vec3 dir, inout float t, BBox bbox){

    vec3 invdir = 1.0/dir;
    vec3 t0 = (bbox.corner-orig)*invdir;
    vec3 nodeMax = bbox.corner-vec3(bbox.size);
    vec3 t1 = (nodeMax-orig)*invdir;

    vec3 tmin = min(t0,t1);
    vec3 tmax = max(t0,t1);
    
    t = max(tmin.x, max(tmin.y, tmin.z));
    float tmax_ = min(tmax.x, min(tmax.y, tmax.z));
    
    return (t < tmax_ && t > 0);
}

vec3 traceIntoVolume(vec3 orig, vec3 dir, Grid grid, uint depth, BBox bbox){
    float t;
    if (traceNode(orig, dir, t, bbox)){
        return vec3(1.0,0.0,0.0);
    }
    else{
        return vec3(0.0,0.0,0.0);
    }
}

void main() {
    float t;
    vec3 nodeMin = pool.transform.xyz;
    float size = pool.transform.w;
    vec3 eye = ubo.eyeDist.xyz;
    outColor.xyz = traceIntoVolume(eye, dir, pool.grids[0], 0, BBox(nodeMin, size));
    //uint vertexColor = pool.grids[0].val.data;
    //vec4 color = vec4((vertexColor & 0x000000FF) >> 0, (vertexColor & 0x0000FF00) >> 8, (vertexColor & 0x00FF0000) >> 16, (vertexColor & 0xFF000000)>> 24);
    //outColor.xyz = color.xyz;
    //outColor.xyz = dir;
    outColor.w = 1.0;
}
