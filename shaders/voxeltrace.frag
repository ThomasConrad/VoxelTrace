#version 450
#extension GL_EXT_scalar_block_layout : require
#define eps 2e-7
#define SHADOWS

// Struct to hold the single cell data from the pool
struct Cell {
  uint data;
};

// Struct to hold the an array of cells and a mipmap val from the pool
struct Grid {
  Cell val;
  Cell cells[8];
};

// Cubic AABB representation struct
struct BBox {
  vec3 pmin;
  float size;
};

struct hitObj {
  vec3 pos;
  vec3 n;
  vec3 col;
};

// Input of rotation, eye and FOV
layout(binding = 0) uniform UniformBufferObject
{
  mat4 ONB;
  vec4 eyeDist;
  float ratio;
  uint framenum;
  float lightIntensity;
  vec3 lightDirection;
  vec3 lightColor;
  vec3 ambientIntensity;
  float ambientColor;
}
ubo;

// SSBO holding the voxel pool
layout(std430, binding = 1) readonly restrict buffer VoxelPool
{
  vec4 transform; // Placement it in the real world
  Grid grids[];
}
pool;

// layout(binding = 2) uniform sampler2D image; //Can be used for voxel textures :)

// Ray direction from vertex shader
layout(location = 0) in vec3 dir;

// Color to be displayed for fragment
layout(location = 0) out vec4 outColor;

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x)
{
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// https://www.shadertoy.com/view/7ttXDl#
vec3 skybox(vec3 dir)
{
  vec3 sun_pos = normalize(ubo.lightDirection);

  vec3 col = vec3(0.);

  vec3 p_sunset_dark[4] = vec3[4](
      vec3(0.3720705374951474, 0.3037080684557225, 0.26548632969565816), vec3(0.446163834012046, 0.39405890487346595, 0.425676737673072),
      vec3(0.16514907579431481, 0.40461292460006665, 0.8799446225003938), vec3(-7.057075230154481e-17, -0.08647963850488945, -0.269042973306185));

  vec3 p_sunset_bright[4] =
      vec3[4](vec3(0.38976745480184677, 0.31560358280318124, 0.27932656874), vec3(1.2874522895367628, 1.0100154283349794, 0.862325457544),
              vec3(0.12605043174959588, 0.23134451619328716, 0.526179948359), vec3(-0.0929868539256387, -0.07334463258550537, -0.192877259333));

  vec3 p_day[4] = vec3[4](
      vec3(0.051010496458305694, 0.09758747153634058, 0.14233364823001612), vec3(0.7216045769411271, 0.8130766810405122, 0.9907063181559062),
      vec3(0.23738746590578705, 0.6037047603190588, 1.279274525377467), vec3(-4.834172446370963e-16, 0.1354589259524697, -1.4694301190050114e-15));

  /* Sky */
  {
    float brightness_a = acos(dot(dir, sun_pos));
    float brightness_d = 1.5 * smoothstep(radians(80.), radians(0.), brightness_a) - .5;

    vec3 p_sunset[4] = vec3[4](mix(p_sunset_dark[0], p_sunset_bright[0], brightness_d), mix(p_sunset_dark[1], p_sunset_bright[1], brightness_d),
                               mix(p_sunset_dark[2], p_sunset_bright[2], brightness_d), mix(p_sunset_dark[3], p_sunset_bright[3], brightness_d));

    float sun_a = acos(dot(sun_pos, vec3(0., 1., 0.)));
    float sun_d = smoothstep(radians(100.), radians(60.), sun_a);

    vec3 a = mix(p_sunset[0], p_day[0], sun_d);
    vec3 b = mix(p_sunset[1], p_day[1], sun_d);
    vec3 c = mix(p_sunset[2], p_day[2], sun_d);
    vec3 d = mix(p_sunset[3], p_day[3], sun_d);

    float sky_a = acos(dot(dir, vec3(0., 1., 0.)));
    float sky_d = smoothstep(radians(90.), radians(60.), sky_a);

    // sin(1/x) suggested by Phillip Trudeau
    col += (b - d) * sin(1. / (vec3(sky_d) / c + 2. / radians(180.) - a)) + d;
  }

  /* Sun */
  float sun_a  = acos(dot(sun_pos, dir));
  vec3 sun_col = .01 * vec3(1., .95, .95) / sun_a;
  col          = max(col + .5 * sun_col, sun_col);

  return col;
}

// Trace a cubic AABB from outside to in
bool traceAABB(vec3 orig, vec3 invdir, inout float t, inout vec3 n, BBox bbox)
{

  vec3 t0      = (bbox.pmin - orig) * invdir;
  vec3 nodeMax = bbox.pmin + vec3(bbox.size);
  vec3 t1      = (nodeMax - orig) * invdir;

  vec3 tmin = min(t0, t1);
  vec3 tmax = max(t0, t1);

  // Normal calculation
  t = tmin.x;
  n = vec3(-(invdir.x > 0 ? 1.0 : -1.0), 0.0, 0.0);
  if (tmin.y > t) {
    t = tmin.y;
    n = vec3(0.0, -(invdir.y > 0 ? 1.0 : -1.0), 0.0);
  }
  if (tmin.z > t) {
    t = tmin.z;
    n = vec3(0.0, 0.0, -(invdir.z > 0 ? 1.0 : -1.0));
  }

  float tmax_ = min(tmax.x, min(tmax.y, tmax.z));

  return (t < tmax_ && t > 0);
}

// Trace a cubic AABB from inside and out
void traceAABBInside(vec3 orig, vec3 invdir, inout float t, inout vec3 n, BBox bbox)
{

  vec3 t0      = (bbox.pmin - orig) * invdir;
  vec3 nodeMax = bbox.pmin + vec3(bbox.size);
  vec3 t1      = (nodeMax - orig) * invdir;

  vec3 tmax = max(t0, t1);

  // Normal calculation
  t = tmax.x;
  n = vec3(-(invdir.x > 0 ? 1.0 : -1.0), 0.0, 0.0);
  if (tmax.y < t) {
    t = tmax.y;
    n = vec3(0.0, -(invdir.y > 0 ? 1.0 : -1.0), 0.0);
  }
  if (tmax.z < t) {
    t = tmax.z;
    n = vec3(0.0, 0.0, -(invdir.z > 0 ? 1.0 : -1.0));
  }

  // float tmax_ = min(tmax.x, min(tmax.y, tmax.z));

  // return (t < tmax_ && t > 0);
}

// Convert last byte in Cell (32 bit int) to uint(8bit)
uint parseType(Cell cell) { return cell.data & 0x000000FF; }

// Convert three first bytes in Cell (3x8 bit uint) to vec3(3x8bit)
vec3 parseCol(Cell cell)
{ // Range between 0 and 1
  return vec3((cell.data & 0xFF000000) >> 24, (cell.data & 0x00FF0000) >> 16, (cell.data & 0x0000FF00) >> 8) *
         0.0039215688593685626983642578125; // multiply by 1/255
}

// Convert three first bytes in Cell (32 bit uint) to uint(24bit)
uint parseidx(Cell cell) { return cell.data >> 8; }

// Check whether a point is inside a bounding box
bool isInside(vec3 point, BBox bbox)
{
  vec3 halfsize = 0.5 * vec3(bbox.size);
  vec3 center   = bbox.pmin + halfsize;
  vec3 dist     = point - center;
  dist *= dist;                                              // distance sqared now
  return all(lessThan(dist, vec3(halfsize.x * halfsize.x))); // are all squared offset coordinates less than the half distance squared
}

// Check whether a point is inside a unitary bounding box
bool isInsideUnit(vec3 point)
{
  vec3 center = vec3(0.5);
  vec3 dist   = point - center;
  dist *= dist;                                   // distance sqared now
  return max(dist.x, max(dist.y, dist.z)) < 0.25; // are all squared offset coordinates less than the half distance squared
}

// Convert an integer representation of a point to a float [0,1]
vec3 idx2point(ivec3 idx)
{
  return vec3(idx) * 0.0000000004656612873077392578125; // 1/(2^30-1)
}

// Convert a float representation of a point [0,1] to an integer
ivec3 point2idx(vec3 point) { return ivec3(2147483647.0 * point); }

// Get the index of a point in a gridarray at a given depth
int getSubindex(ivec3 intPos, int depth)
{
  int c      = 30 - depth;
  int mask   = 1 << c;
  ivec3 bits = (intPos & mask) >> c;
  return bits.x * 4 + bits.y * 2 + bits.z;
}

// Either return the largest safe bbox for a point or the color if the point is in a voxel
bool bboxOrCol(vec3 pos, inout vec4 val)
{
  ivec3 intpos = point2idx(pos); // same representation but as int. Avoids future divisions for speed
  uint gridIdx = 0;
  int depth    = 0;
  while (true) {
    int nodeIdx = getSubindex(intpos, depth);
    Grid grid   = pool.grids[gridIdx];
    Cell node   = grid.cells[nodeIdx];
    uint w      = parseType(node);
    if (w == 1) { // If it's 1 we print the color
      vec3 cellVal = parseCol(node);
      val          = vec4(cellVal, 1.0); // Divide by 255 for between 0 and 1
      return false;
    }
    else if (w == 0) { // If it's zero it's empty and the bbox is safe
      int c      = 30 - depth;
      vec3 start = idx2point((intpos >> c) << c);
      float size = float(1 << c) * 0.0000000004656612873077392578125; // 1/(2^30-1)
      val        = vec4(start, size);
      return true;
    }
    gridIdx += parseidx(node); // Step to next level
    depth += 1;
    // If it's 2 we must go deeper
  }
}

bool trace(vec3 pos, vec3 dir, vec3 invdir, inout hitObj hit)
{
  float t = 0.;
  vec3 n  = vec3(0.);
  vec4 col;
  while (true) {
    if (!bboxOrCol(pos, col)) {
      break;
    }
    BBox safe_bbox = BBox(col.xyz, col.w);
    traceAABBInside(pos, invdir, t, n, safe_bbox);
    pos += t * dir - n * 1e-7;
    if (!isInsideUnit(pos)) {
      return false; // Return background color if we traced out
    }
  }
  hit.pos = pos + n * 1e-6;
  hit.n   = n;
  hit.col = col.xyz;
  return true;
}

vec3 intersect(vec3 orig, vec3 dir, BBox bbox)
{
  float t     = 0.;
  vec3 n      = vec3(0.);
  vec3 invdir = 1.0 / dir;
  if (isInside(orig, bbox) || traceAABB(orig, invdir, t, n, bbox)) { // Either inside or trace to the bounding box
    vec3 nodeMin = bbox.pmin;
    float size   = bbox.size;
    vec3 pos     = (orig + t * dir - t * n * eps - nodeMin) / size; // step into node and scale to [0,1]
    hitObj hit;
    if (trace(pos, dir, invdir, hit)) {
      vec3 col       = hit.col;
      vec3 shadowCol = vec3(0.);
#if defined(SHADOWS)
      vec3 nn = vec3(0.);
      // check border
      if (!isInside(hit.pos * size + nodeMin + n * 1e-4, bbox) && dot(ubo.lightDirection, n) > 0.) {
        shadowCol = ubo.lightColor * ubo.lightIntensity * dot(normalize(n), ubo.lightDirection);
      }
      // check inside
      else if (!trace(hit.pos, ubo.lightDirection, 1. / ubo.lightDirection, hit)) {
        shadowCol = ubo.lightColor * ubo.lightIntensity * dot(hit.n, ubo.lightDirection);
      }
#endif
      return (shadowCol + ubo.ambientColor * ubo.ambientIntensity) * vec3(col);
    }
  }
  return skybox(normalize(dir)); // Background color
}

void main()
{
  float t;
  vec3 nodeMin = pool.transform.xyz;
  float size   = pool.transform.w;
  vec3 eye     = ubo.eyeDist.xyz;

  outColor.xyz = aces(intersect(eye, dir, BBox(nodeMin, size)));
  // outColor.xyz = -vec3(dir.z);
  // outColor.xyz = dir;
  outColor.w = 1.0;
}
