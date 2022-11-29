#version 450
#extension GL_EXT_scalar_block_layout: require
#define eps 1e-7

struct Cell {
    uint data;
};

struct Grid {
    Cell val;
    Cell cells[8];
};

struct BBox {
    vec3 pmin;
    float size;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 ONB;
    vec4 eyeDist;
    float ratio;
} ubo;

layout(std430, binding = 1) readonly restrict buffer VoxelVolume{
    vec4 transform; //Placement it in the real world
    Grid grids[];
} pool;

//layout(binding = 2) uniform sampler2D image; //Can be used for voxel textures :)

layout(location = 0) in vec3 dir;

layout(location = 0) out vec4 outColor;

bool traceAABB(vec3 orig, vec3 invdir, inout float t, inout vec3 n, BBox bbox){

    vec3 t0 = (bbox.pmin-orig)*invdir;
    vec3 nodeMax = bbox.pmin+vec3(bbox.size);
    vec3 t1 = (nodeMax-orig)*invdir;

    vec3 tmin = min(t0,t1);
    vec3 tmax = max(t0,t1);
    
    //Normal calculation
    t = tmin.x; 
    n = vec3(-(invdir.x > 0 ? 1.0 : -1.0),0.0,0.0);
    if (tmin.y > t){
        t = tmin.y; 
        n = vec3(0.0,-(invdir.y > 0 ? 1.0 : -1.0),0.0);
    }
    if (tmin.z > t){
        t = tmin.z; 
        n = vec3(0.0,0.0,-(invdir.z > 0 ? 1.0 : -1.0));
    }

    float tmax_ = min(tmax.x, min(tmax.y, tmax.z));
    
    return (t < tmax_ && t > 0);
}

bool traceAABBInside(vec3 orig, vec3 invdir, inout float t, inout vec3 n){

    vec3 t0 = -orig*invdir;
    vec3 t1 = invdir-orig*invdir;

    vec3 tmax = min(t0,t1);
    vec3 tmin = max(t0,t1);
    
    //Normal calculation
    t = tmin.x; 
    n = vec3(-(invdir.x > 0 ? 1.0 : -1.0),0.0,0.0);
    if (tmin.y > t){
        t = tmin.y; 
        n = vec3(0.0,-(invdir.y > 0 ? 1.0 : -1.0),0.0);
    }
    if (tmin.z > t){
        t = tmin.z; 
        n = vec3(0.0,0.0,-(invdir.z > 0 ? 1.0 : -1.0));
    }

    float tmax_ = min(tmax.x, min(tmax.y, tmax.z));
    
    return (t < tmax_ && t > 0);
}

uint parseType(Cell cell){
    return cell.data & 0x000000FF;
}

vec3 parseCol(Cell cell){ //Range between 0 and 1
    return vec3((cell.data & 0xFF000000) >> 24, (cell.data & 0x00FF0000) >> 16, (cell.data & 0x0000FF00) >> 8)*0.00392156862745;
}

uint parseidx(Cell cell){
    return cell.data >> 8;
}

bool isInside(vec3 orig, BBox bbox){
    return orig.x > bbox.pmin.x 
        && orig.y > bbox.pmin.y 
        && orig.z > bbox.pmin.z 
        && orig.x < bbox.pmin.x + bbox.size
        && orig.y < bbox.pmin.y + bbox.size
        && orig.z < bbox.pmin.z + bbox.size;
}

vec3 idx2point(uvec3 idx){
    return vec3(idx)*0.00000000023283064365386962890625;
}

uvec3 point2idx(vec3 point){
    return uvec3(4294967295.0*point);
}

uint getSubindex(uvec3 uintPos, uint depth){
    uint mask = 1 << (31 - depth); 
    int x_bit, y_bit, z_bit;
    x_bit = int(0 != (uintPos.x & mask));
    y_bit = int(0 != (uintPos.y & mask));
    z_bit = int(0 != (uintPos.z & mask));
    return x_bit*4+y_bit*2+z_bit;
}


bool bboxOrCol(uint gridIdx, uint depth, vec3 pos, inout vec4 val){
    uvec3 uintpos = point2idx(pos); //same representation but as uint. Avoids future divisions for speed
    while(true){
        uint nodeIdx = getSubindex(uintpos, depth); 
        Grid grid = pool.grids[gridIdx];
        Cell node = grid.cells[nodeIdx];
        uint w = parseType(node);
        if (w == 1 ){ //If it's 1 we print the color
            vec3 cellVal = parseCol(node);
            val = vec4(cellVal,1.0); //Divide by 255 for between 0 and 1
            return true;
        }
        else if (w == 0 ){ // If it's zero it's empty and the bbox is safe
            uint c = 31 - depth;
            uvec3 start = (uintpos >> c)<<c;
            return false;
        }
        gridIdx+=parseidx(node); //Step to next level
        depth += 1;
        // If it's 2 we must go deeper
    }
}

void trace(vec3 pos, vec3 dir, vec3 invdir){
    vec4 val;
    bboxOrCol(0, 0, pos, val);
}

vec3 intersect(vec3 orig, vec3 dir, BBox bbox){
    float t = 0.;
    vec3 n = vec3(0.);
    vec3 invdir = 1.0/dir;
    if (isInside(orig, bbox) || traceAABB(orig, invdir, t, n, bbox)){ // Either inside or trace to the bounding box
        vec3 nodeMin = bbox.pmin;
        float size = bbox.size;
        vec3 pos = (orig+t*dir-n*eps-nodeMin)/size; //step into node and scale to [0,1]
        
        vec3 invdir = 1.0/dir;

        //trace(pos, dir, invdir);
        
        return vec3(1.0,0.0,0.0);
    }
    else{
        return vec3(0.0,1.0,0.0);
    }
}

void main() {
    float t;
    vec3 nodeMin = pool.transform.xyz;
    float size = pool.transform.w;
    vec3 eye = ubo.eyeDist.xyz;
    



    outColor.xyz = intersect(eye, dir, BBox(nodeMin, size));
    //outColor.xyz = -vec3(dir.z);
    //outColor.xyz = dir;
    outColor.w = 1.0;
}
