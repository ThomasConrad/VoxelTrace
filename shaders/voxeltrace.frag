#version 450
#extension GL_EXT_scalar_block_layout: require
#define eps 2e-7

//Struct to hold the single cell data from the pool
struct Cell {
    uint data;
};

//Struct to hold the an array of cells and a mipmap val from the pool
struct Grid {
    Cell val;
    Cell cells[8];
};

//Cubic AABB representation struct
struct BBox {
    vec3 pmin;
    float size;
};

//Input of rotation, eye and FOV
layout(binding = 0) uniform UniformBufferObject {
    mat4 ONB;
    vec4 eyeDist;
    float ratio;
} ubo;

//SSBO holding the voxel pool
layout(std430, binding = 1) readonly restrict buffer VoxelPool{
    vec4 transform; //Placement it in the real world
    Grid grids[];
} pool;

//layout(binding = 2) uniform sampler2D image; //Can be used for voxel textures :)

//Ray direction from vertex shader
layout(location = 0) in vec3 dir;

//Color to be displayed for fragment
layout(location = 0) out vec4 outColor;

//Trace a cubic AABB from outside to in 
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

//Trace a cubic AABB from inside and out
void traceAABBInside(vec3 orig, vec3 invdir, inout float t, inout vec3 n, BBox bbox){

    vec3 t0 = (bbox.pmin-orig)*invdir;
    vec3 nodeMax = bbox.pmin+vec3(bbox.size);
    vec3 t1 = (nodeMax-orig)*invdir;

    vec3 tmax = max(t0,t1);
    
    //Normal calculation
    t = tmax.x; 
    n = vec3(-(invdir.x > 0 ? 1.0 : -1.0),0.0,0.0);
    if (tmax.y < t){
        t = tmax.y; 
        n = vec3(0.0,-(invdir.y > 0 ? 1.0 : -1.0),0.0);
    }
    if (tmax.z < t){
        t = tmax.z; 
        n = vec3(0.0,0.0,-(invdir.z > 0 ? 1.0 : -1.0));
    }

    //float tmax_ = min(tmax.x, min(tmax.y, tmax.z));
    
    //return (t < tmax_ && t > 0);
}

//Convert last byte in Cell (32 bit int) to uint(8bit)
uint parseType(Cell cell){
    return cell.data & 0x000000FF;
}

//Convert three first bytes in Cell (3x8 bit uint) to vec3(3x8bit)
vec3 parseCol(Cell cell){ //Range between 0 and 1
    return vec3((cell.data & 0xFF000000) >> 24, (cell.data & 0x00FF0000) >> 16, (cell.data & 0x0000FF00) >> 8)*0.0039215688593685626983642578125; //multiply by 1/255
}

//Convert three first bytes in Cell (32 bit uint) to uint(24bit)
uint parseidx(Cell cell){
    return cell.data >> 8;
}

//Check whether a point is inside a bounding box
bool isInside(vec3 point, BBox bbox){
    vec3 halfsize = 0.5*vec3(bbox.size);
    vec3 center = bbox.pmin+halfsize;
    vec3 dist = point-center;
    dist *= dist; //distance sqared now
    return all(lessThan(dist,vec3(halfsize.x*halfsize.x))); //are all squared offset coordinates less than the half distance squared
}

//Check whether a point is inside a unitary bounding box
bool isInsideUnit(vec3 point){
    vec3 center = vec3(0.5);
    vec3 dist = point-center;
    dist *= dist; //distance sqared now
    return max(dist.x,max(dist.y,dist.z)) < 0.25; //are all squared offset coordinates less than the half distance squared
}

//Convert an integer representation of a point to a float [0,1]
vec3 idx2point(ivec3 idx){
    return vec3(idx)*0.0000000004656612873077392578125;
}

//Convert a float representation of a point [0,1] to an integer
ivec3 point2idx(vec3 point){
    return ivec3(2147483647.0*point);
}

//Get the index of a point in a gridarray at a given depth
int getSubindex(ivec3 intPos, int depth){
    int c = 30 - depth;
    int mask = 1 << c; 
    ivec3 bits = (intPos & mask) >> c;
    return bits.x*4+bits.y*2+bits.z;
}

//Either return the largest safe bbox for a point or the color if the point is in a voxel
bool bboxOrCol(vec3 pos, inout vec4 val){
    ivec3 intpos = point2idx(pos); //same representation but as int. Avoids future divisions for speed
    uint gridIdx = 0;
    int depth = 0;
    while(true){
        int nodeIdx = getSubindex(intpos, depth); 
        Grid grid = pool.grids[gridIdx];
        Cell node = grid.cells[nodeIdx];
        uint w = parseType(node);
        if (w == 1 ){ //If it's 1 we print the color
            vec3 cellVal = parseCol(node);
            val = vec4(cellVal,1.0); //Divide by 255 for between 0 and 1
            return false;
        }
        else if (w == 0 ){ // If it's zero it's empty and the bbox is safe
            int c = 30 - depth;
            vec3 start = idx2point((intpos >> c) << c);
            float size = float(1 << c) * 0.0000000004656612873077392578125;
            val = vec4(start,size);
            return true;
        }
        gridIdx+=parseidx(node); //Step to next level
        depth += 1;
        // If it's 2 we must go deeper
    }
}

bool trace(vec3 pos, vec3 dir, vec3 invdir, inout vec4 col){
    float t = 0.;
    vec3 n = vec3(0.);
    while (bboxOrCol(pos, col)){
        BBox safe_bbox = BBox(col.xyz, col.w);
        traceAABBInside(pos, invdir, t, n, safe_bbox);
        pos += t*dir-n*eps; 
        if (!isInsideUnit(pos)){
            return false; //Return background color if we traced out
        }
    }
    return true;
}

vec3 intersect(vec3 orig, vec3 dir, BBox bbox){
    float t = 0.;
    vec3 n = vec3(0.);
    vec3 invdir = 1.0/dir;
    if (isInside(orig, bbox) || traceAABB(orig, invdir, t, n, bbox)){ // Either inside or trace to the bounding box
        vec3 nodeMin = bbox.pmin;
        float size = bbox.size;
        vec3 pos = (orig+t*(dir-n*eps)-nodeMin)/size; //step into node and scale to [0,1]
        vec4 col;
        if(trace(pos, dir, invdir, col)){
            return vec3(col);
        }
    }
    return vec3(0.0,0.0,0.0); //Background color
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
