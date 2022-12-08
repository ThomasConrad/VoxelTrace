#pragma once
#include "3d_prims.h"
#include "scene.h"
#include <cstdint>
#include "voxelspace.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <omp.h>
#include <OpenImageDenoise/oidn.hpp>
#include "stb_image.h"
#include "stb_image_write.h"

#define _USE_MATH_DEFINES
#include <math.h>
enum Keys {
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_RIGHT,
    MOVE_LEFT,
    MOVE_UPWARDS,
    MOVE_DOWNWARDS,
    ROTATE_CCW,
    ROTATE_CW,
    PLACE_BLOCK,
    REMOVE_BLOCK,
    RENDER_MODE_SIMPLE,
    RENDER_MODE_GLOBAL,
    RENDER_MODE_NORMALS,
    RENDER_MODE_PIXELIDX,
    RENDER_MODE_AO,
    SCREENSHOT,
    TOGGLEDENOISE
};

enum RenderMode {
    SIMPLE,
    GLOBAL_ILLUM,
    NORMALS,
    PIXELIDX,
    AO
};

inline float mean(vec3 v) {
    return (v.x + v.y + v.z) / 3.0f;
};

// Given a direction vector v sampled on the hemisphere
// over a surface point with the z-axis as its normal,
// this function applies the same rotation to v as is
// needed to rotate the z-axis to the actual normal
// [Frisvad, Journal of Graphics Tools 16, 2012;
//  Duff et al., Journal of Computer Graphics Techniques 6, 2017].
inline void rotate_to_normal(const vec3 normal, vec3 &v)
{
  const float sign = copysignf(1.0f, normal.z);
  const float a = -1.0f/(1.0f + fabsf(normal.z));
  const float b = normal.x*normal.y*a;
  v =   vec3(1.0f + normal.x*normal.x*a, b, -sign*normal.x)*v.x 
      + vec3(sign*b, sign*(1.0f + normal.y*normal.y*a), -normal.y)*v.y 
      + normal*v.z;
}

typedef struct {
    uint64_t  state;
} prng_state;

static inline uint64_t prng_u64(prng_state *const p)
{
    uint64_t  state = p->state;
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    p->state = state;
    return state * UINT64_C(2685821657736338717);
}

static inline double prng_one(prng_state *p)
{
    return prng_u64(p) / 18446744073709551616.0;
}

class RayTracer{
    Scene scene;
    VoxelSpace<Voxel>* model;
    mat3 ONB; //RIGHT, UP, BACKWARDS
    vec3 speed;
    vec3 eye;
    float fov;
    float s;
    float lastx, lasty;
    float* env;
    int texWidth, texHeight, texChannels;
    float *accumBuffer;
    float *normalBuffer;
    float *albedoBuffer;

    //Stuff for cellular automaton, ignore if not used ;)
    float time_since_tick;
    float secs_per_tick;

    float64 framenum = 0;
    prng_state* states;
    oidn::DeviceRef device;
    oidn::FilterRef filter;
    std::pair<int,bool> keys[17];
    void init_controls();
    bool denoiserOn = false;
    vec3 getDir(float i, float j);
    vec3 getDirJittered(uint32_t i, uint32_t j, vec2 offset);

    //Returns an HDR color of whatever was hit. If nothing
    //was hit, returns the background color.
    vec3 trace(Ray r);
    //Returns an HDR color of whatever was hit. If nothing
    //was hit, returns the background color.
    vec3 trace(Ray r, HitInfo &hit);

    bool trace_new(Ray r, HitInfo &hit) const;
    
    RenderMode render_mode;
    vec3 voxelShade(const HitInfo& hit);
    vec3 sampleEnvironment(Ray r);

    public:
        uint32_t width, height, texchannels, imageSize;

        //constructors
        RayTracer() {}
        RayTracer(uint32_t _width, uint32_t _height){
            init(_width, _height);
        }
        //Destructor is in the .cpp file, had to do that to make it work
        ~RayTracer();

        void render(uint8_t pixels[], float time);
        void rotate(float xp, float yp);
        void init(uint32_t _width, uint32_t _height);
        void keypress(int key, int action);
        void input_update(float time);
};