#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "raytracer.h"
#include <cstdlib>
#include <glm/common.hpp>
#include <glm/fwd.hpp>
#include <iostream>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/gtx/string_cast.hpp>
#include <limits>
#include <math.h>
#include "scene.h"

#define par true

vec3 voxelNormalsShade(HitInfo hit) {
    if (hit.hit == false) {
        return vec3(0, 0, 0);
    }
    vec3 color = hit.n*0.5f+0.5f;
    return color;
}

vec3 voxelDepthShade(HitInfo hit) {
    vec3 color = vec3(hit.t)*120.0f;
    return color;
}

vec3 voxelPositionShade(HitInfo hit) {
    vec3 color = hit.p;
    return color;
}

//TONEMAPPING
vec3 aces_approx(vec3 v)
{
    v *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);
}
//Schlicks approximation. Eta is ior2/ior1
float schlick(vec3 normal, vec3 incident, float eta){
    float r0 = (1-eta) / (1+eta);
    r0 = r0*r0;
    float cos = dot(normal, incident);
    return r0 ;
}

vec3 RayTracer::getDir(float i, float j){
    vec3 dir;
    dir.x = (float)i - (float)width / 2.0;
    dir.y = ((float)j - (float)height/ 2.0);
    dir.z = 1/s;
    return ONB*normalize(dir);
}
vec3 RayTracer::getDirJittered(uint32_t i, uint32_t j, vec2 offset){
    return getDir(i+offset.x, j+offset.y);
}

vec3 voxelSimpleShade(HitInfo hit) {
    vec3 color = hit.obj.albedo;
    if (hit.n.x < 0) { //x is left-to-right (positive left)
        color *= 0.9;
    }
    if (hit.n.y < 0) { //y is up-and-down, like Minecraft, I hate it (positive up)
        color *= 0.4;
    }
    if (hit.n.z > 0) { //z is forwards-backwards (positive into screen)
        color *= 0.9;
    }

    if (hit.n.y > 0) {
        color *= vec3(0.0,60.0,60.0);
        color.r = 1.0;
    }

    return color;
}

vec3 RayTracer::voxelShade(const HitInfo& hit) {
    float eps = 1e-3;

    if (hit.obj.emission > 0.0f) {
        return hit.obj.albedo * hit.obj.emission;
    }

    //Shadow ray
    Ray shadow_ray = Ray(hit.p + eps*hit.n, scene.directional_light_dir);
    HitInfo shadow_hit;
    bool in_shadow = trace_new(shadow_ray, shadow_hit);

    vec3 direct = in_shadow ? vec3(0,0,0) : hit.obj.albedo * scene.directional_light_col * scene.directional_light * dot(hit.n, scene.directional_light_dir);
    //vec3 ambient = hit.obj.col*scene.ambient_light_col*scene.ambient_light;
    vec3 result = direct;// + ambient;
    return result;
}
vec3 RayTracer::sampleEnvironment(Ray r) {
    float u = 0.5f + atan2(r.direction.x,r.direction.z)/(2.0f*M_PI);
    float v = 0.5f + asin(-r.direction.y)/M_PI;
    uint i = (uint)(u * texWidth);
    uint j = (uint)(v * texHeight);
    int idx = (j*texWidth+i)*texChannels;

    return vec3(env[idx],env[idx+1],env[idx+2]);
}

vec3 refract(const vec3& uv, const vec3& n, float etai_over_etat) {
    float cos_theta = fmin(dot(-uv, n), 1.0);
    vec3 r_out_perp =  etai_over_etat * (uv + cos_theta*n);
    vec3 r_out_parallel = -sqrt(fabs(1.0f - dot(r_out_perp,r_out_perp))) * n;
    return r_out_perp + r_out_parallel;
}

vec3 RayTracer::trace(Ray r){
    HitInfo hit;
    return trace(r, hit);
}

vec3 RayTracer::trace(Ray r, HitInfo &hit){
    vec3 light_col;
    if (model->intersect(r, hit)) {
        light_col = voxelShade(hit); 
        hit.hit = true;
    } else {
        light_col = 3.0f*sampleEnvironment(r);
        hit.hit = false;
    }
    hit.ray_color = light_col;
    return light_col; // DO SOME HDR TONEMAPPING
}


bool RayTracer::trace_new(Ray r, HitInfo &hit) const {
    if (model->intersect(r, hit)) {
        hit.hit = true;
    } else {
        hit.hit = false;
    }
    return hit.hit;
}


void RayTracer::keypress(int key, int action){
    if (!(key == keys[SCREENSHOT].first || key == keys[TOGGLEDENOISE].first)) {
        framenum = 0;
    }
    for(auto i = 0; i < sizeof(keys)/sizeof(keys[0]); ++i){
        if (keys[i].first == key){
            keys[i].second = action;
            return;
        }
    }
}
void RayTracer::input_update(float delta_time) {
    vec3 change = (float)keys[MOVE_RIGHT].second*ONB[0] -
                  (float)keys[MOVE_LEFT].second*ONB[0] +
                  (float)keys[MOVE_UPWARDS].second*ONB[1] -
                  (float)keys[MOVE_DOWNWARDS].second*ONB[1] +
                  (float)keys[MOVE_FORWARD].second*ONB[2] -
                  (float)keys[MOVE_BACKWARD].second*ONB[2] ;

    eye += 0.1f*change*delta_time*length(model->bounding_box.size());
    float dAngle = (float)keys[ROTATE_CW].second*delta_time - (float)keys[ROTATE_CCW].second*delta_time;
    ONB = glm::rotate(dAngle, ONB[2])*mat4(ONB);

    #ifndef WATERSIM
    //Regular scene
    if (keys[REMOVE_BLOCK].second) {
        vec3 forward = ONB[2];
        HitInfo hit;
        if (model->intersect(Ray(eye, forward), hit)) {
            model->remove_voxel_at_point(hit.p);
        }
    } else if (keys[PLACE_BLOCK].second) {
        vec3 forward = ONB[2];
        vec3 place_pos = eye + forward*5.0f;
        Voxel new_voxel = Voxel(vec3(1.0,1.0,1.0),10.0f);
        model->place_voxel_at_point(place_pos,new_voxel);
    }
    #else
    //Water sim
    if (keys[REMOVE_BLOCK].second) {
        vec3 forward = ONB[2];
        HitInfo hit;
        uvec3 hit_index;
        if (model->intersect(Ray(eye, forward), hit)) {
            model->try_point_to_octree_index(hit.p, hit_index);
            water_sim->remove_voxel(hit_index);
        }
    } else if (keys[PLACE_BLOCK].second) {
        vec3 forward = ONB[2];
        vec3 place_pos = eye + forward*5.0f;
        uvec3 hit_index;
        if (model->try_point_to_octree_index(place_pos, hit_index)) {
            water_sim->place_water_source(hit_index);
        }
    }
    #endif
    if (keys[TOGGLEDENOISE].second) { denoiserOn = !denoiserOn; }
    if (keys[RENDER_MODE_SIMPLE].second) render_mode = RenderMode::SIMPLE;
    if (keys[RENDER_MODE_GLOBAL].second) render_mode = RenderMode::GLOBAL_ILLUM;
    if (keys[RENDER_MODE_NORMALS].second) render_mode = RenderMode::NORMALS;
    if (keys[RENDER_MODE_PIXELIDX].second) render_mode = RenderMode::PIXELIDX;
    if (keys[RENDER_MODE_AO].second) render_mode = RenderMode::AO;
}

void RayTracer::rotate(float xp, float yp) {
    if (lastx != 0.0 && lasty != 0.0){
        float theta = -(xp-lastx)*0.001f;
        float phi = -(yp-lasty)*0.001f;
        ONB = glm::rotate(theta, ONB[1])*glm::rotate(phi, ONB[0])*mat4(ONB);
    }
    lastx = xp;
    lasty = yp;
    framenum = 0;
}


//2D sampling for jitters
float sample(prng_state *p) {
    return prng_one(p);
}

vec2 sample2D(prng_state *p){
    float x = sample(p);
    float y = sample(p);
    return vec2(x,y);
}

vec2 generate_jitter_offset(prng_state *p) {
    vec2 offset = sample2D(p);
    offset -= 0.5f;
    return offset;
}

const int max_bounces = 16;
void RayTracer::render(uint8_t *pixels, float time){
    if (keys[SCREENSHOT].second){
        keys[SCREENSHOT].second = false;
        std::cout << "Saving screenshot..." << std::endl;
        stbi_flip_vertically_on_write(true);
        stbi_write_png("screenshot.png", width, height, 4, pixels, width*4);
    }

    #ifndef nopar
    #pragma omp parallel for
    #endif
    for (int32_t i = 0; i < width; i++){ //LOOP BEGINS
    for (int32_t j = 0; j < height; j++){
    vec3 col;
    int idx = (j*width+i)*texchannels;
    //Each thread has an allowed 100 unique samples

    switch (render_mode) {

        case (RenderMode::SIMPLE):
        {
            //setup and trace
            Ray r = Ray(eye, getDir(i,j));
            HitInfo hit;
            trace_new(r,hit);
            col = hit.hit ? 255.0f*hit.obj.albedo : vec3(0.0f);
            col.x = clamp(col.x, 0.0f, 255.0f);
            col.y = clamp(col.y, 0.0f, 255.0f);
            col.z = clamp(col.z, 0.0f, 255.0f);
        }
        break;

        case (RenderMode::NORMALS):
        {
            //setup and trace
            Ray r = Ray(eye, getDir(i,j));
            HitInfo hit;
            trace(r,hit)*255.0f;
            col = voxelNormalsShade(hit)*255.0f;
        }
        break;

        case (RenderMode::PIXELIDX):
        {
            //setup and trace
            col = vec3((i*255/width),(j*255/width),0.0);
        }
        break;

        case (RenderMode::AO):
        {
            prng_state *p = &states[omp_get_thread_num()];
            //we always trace the first ray and direct light
            vec2 offset = generate_jitter_offset(p);
            Ray r = Ray(eye, getDirJittered(i,j,offset));
            HitInfo GIhit;
            vec3 light_col = vec3(1.0f);
            trace(r,GIhit);
            if (GIhit.hit) {
                //Sample cosine hemisphere
                float phi = 2*M_PI*sample(p);
                float theta = acos(sqrt(sample(p)));
                vec3 dir = vec3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));

                //Rotate to normal
                rotate_to_normal(GIhit.n,dir);

                //Trace
                r = Ray(GIhit.p+GIhit.n*1e-4f, dir);
                GIhit = HitInfo();
                trace(r, GIhit); //multiply by albedo of last hit to attenuate
                if (GIhit.hit) {
                    light_col = vec3(0.0f);
                }
                else{
                    light_col = vec3(1.0f);
                }
            }
            light_col = light_col*255.0f;
            //Update accumBufferulation buffer
            int idx = (j*width+i)*3;
            accumBuffer[idx] = accumBuffer[idx]   * (framenum/(framenum+1.0)) + (uint8_t)light_col.r * (1/(framenum+1.0));
            accumBuffer[idx+1] = accumBuffer[idx+1] * (framenum/(framenum+1.0)) + (uint8_t)light_col.g * (1/(framenum+1.0));
            accumBuffer[idx+2] = accumBuffer[idx+2] * (framenum/(framenum+1.0)) + (uint8_t)light_col.b * (1/(framenum+1.0));
            
            

            col.r = accumBuffer[idx];
            col.g = accumBuffer[idx+1];
            col.b = accumBuffer[idx+2];
        }
        break;

        case (RenderMode::GLOBAL_ILLUM):
        {
            int idx = (j*width+i)*3;
            prng_state *p = &states[omp_get_thread_num()];
            //we always trace the first ray and direct light
            vec2 offset = generate_jitter_offset(p);
            Ray r = Ray(eye, getDirJittered(i,j,offset));
            HitInfo GIhit;
            vec3 direct = trace(r,GIhit);
            vec3 light_col = vec3(0.0f);
            // One day this will all be recursive
            if (GIhit.hit) { //If it did hit something, it can bound
                vec3 attenuated = GIhit.obj.albedo;
                //Update normal buffer
                normalBuffer[idx] = GIhit.n.x;
                normalBuffer[idx+1] = GIhit.n.y;
                normalBuffer[idx+2] = GIhit.n.z;
                //Update albedo buffer
                albedoBuffer[idx] = GIhit.obj.albedo.r;
                albedoBuffer[idx+1] = GIhit.obj.albedo.g;
                albedoBuffer[idx+2] = GIhit.obj.albedo.b;

                int bounces = 0;
                // 1-mean(albedo) is the probability of termination
                if (GIhit.obj.emission > 0.0f){ // First bounce hit an emissive object
                    light_col = GIhit.obj.emission*GIhit.obj.albedo;
                }
                else{
                    //now we trace the rest of the rays monte-carlo style
                    while (bounces < max_bounces){
                        //Here we sample the bsdf of the hit object to get an out direction:
                        vec3 dir;
                        //Sample cosine hemisphere
                        if (GIhit.obj.matType == 0){ //Diffuse
                            if (sample(p) < pow(mean(GIhit.obj.albedo),3)){ //A lot of paths end up dead here
                                break;
                            }
                            float phi = 2*M_PI*sample(p);
                            float theta = acos(sqrt(sample(p)));
                            dir = vec3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));

                            //Rotate to normal
                            rotate_to_normal(GIhit.n,dir);
                            r = Ray(GIhit.p+GIhit.n*1e-4f, dir);

                        }
                        else if (GIhit.obj.matType == 1){ //mirror
                            //Reflect
                            dir = reflect(r.direction, GIhit.n);
                            vec fuzz = vec3((sample2D(p)-0.5f)*0.1f,0.0f);
                            rotate_to_normal(dir,fuzz);
                            dir = dir + fuzz;
                            r = Ray(GIhit.p+GIhit.n*1e-4f, normalize(dir));
                        }
                        else if (GIhit.obj.matType == 2){ //glass
                            //Refract
                            vec3 normal = GIhit.inside ? -GIhit.n : GIhit.n;
                            float eta = GIhit.inside ? GIhit.obj.ior : 1/GIhit.obj.ior;
                            r.direction = normalize(r.direction);
                            float R = schlick(normal, r.direction, 1/eta);

                            double cos_theta = fmin(dot(-r.direction, GIhit.n), 1.0);
                            double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

                            bool cannot_refract = eta * sin_theta > 1.0;

                            if (cannot_refract || R > sample(p))
                                r = Ray(GIhit.p+GIhit.n*1e-4f, reflect(r.direction, GIhit.n));
                            else 
                                r = Ray(GIhit.p-GIhit.n*1e-4f, refract(r.direction, GIhit.n, eta));

                    
                        }
                        else if (GIhit.obj.matType == 3){ //emissive
                            r = Ray(GIhit.p+GIhit.n*1e-4f, dir);
                        }
                        
                        GIhit = HitInfo();
                        trace(r, GIhit);
                        attenuated *= GIhit.obj.albedo; //multiply by albedo to attenuate

                        if (!GIhit.hit){ // We traced out, shade by environment
                            light_col = min(GIhit.ray_color*attenuated,50.0f); //Clamp to avoid fireflies. Loses energy but ehh
                            break;
                        }
                        if (GIhit.obj.emission > 0.0f){ // We hit an emissive object
                            light_col = GIhit.obj.emission*attenuated;
                            break;
                        }
                        direct += GIhit.ray_color*attenuated;
                        //Probability of doing another bounce
                        
                        
                        bounces++;
                    }
                    
                }
            }
            else{
                direct = vec3(1.0);
                normalBuffer[idx] = 0.0f;
                normalBuffer[idx+1] = 0.0f;
                normalBuffer[idx+2] = 0.0f;
                //Update albedo buffer
                albedoBuffer[idx] = 0.0f;
                albedoBuffer[idx+1] = 0.0f;
                albedoBuffer[idx+2] = 0.0f;
            }
            light_col += direct;
            accumBuffer[idx]   = accumBuffer[idx]   * (framenum/(framenum+1.0)) + light_col.r * (1/(framenum+1.0));
            accumBuffer[idx+1] = accumBuffer[idx+1] * (framenum/(framenum+1.0)) + light_col.g * (1/(framenum+1.0));
            accumBuffer[idx+2] = accumBuffer[idx+2] * (framenum/(framenum+1.0)) + light_col.b * (1/(framenum+1.0));
            col.r = accumBuffer[idx];
            col.g = accumBuffer[idx+1];
            col.b = accumBuffer[idx+2];
            col = aces_approx(col)*255.0f;
        }
        break;
        
    }
    pixels[idx]   = (uint8_t)col.r;
    pixels[idx+1] = (uint8_t)col.g;
    pixels[idx+2] = (uint8_t)col.b;
    pixels[idx+3] = (uint8_t)255;

    }
    } // LOOP ENDS
    //DENOISING
    if (render_mode == RenderMode::GLOBAL_ILLUM && denoiserOn){
        // Create a denoising filter
        float *denoised = new float[width*height*3];

        filter.setImage("color",  accumBuffer,  oidn::Format::Float3, width, height);
        filter.setImage("normal", normalBuffer, oidn::Format::Float3, width, height);
        filter.setImage("albedo", albedoBuffer, oidn::Format::Float3, width, height);
        filter.setImage("output", denoised, oidn::Format::Float3, width, height);
        filter.set("hdr", true); // image is HDR
        filter.commit();

        // Filter the image
        filter.execute();
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None)
            std::cout << "Error: " << errorMessage << std::endl;

        for (uint32_t i = 0; i < width; i++){ //LOOP BEGINS
        for (uint32_t j = 0; j < height; j++){
        int idx3 = (j*width+i)*3;
        int idx4 = (j*width+i)*4;
        
        // vec3 col;
        // if (i > width/2)
        //     col = vec3(accumBuffer[idx3], accumBuffer[idx3+1], accumBuffer[idx3+2]);
        // else
        //     col = vec3(denoised[idx3], denoised[idx3+1], denoised[idx3+2]);
        
        vec3 col = vec3(denoised[idx3], denoised[idx3+1], denoised[idx3+2]);
        col = aces_approx(col)*255.0f;
        pixels[idx4]   = (uint8_t)col.r;
        pixels[idx4+1] = (uint8_t)col.g;
        pixels[idx4+2] = (uint8_t)col.b;
        pixels[idx4+3] = (uint8_t)255;
        }
        } // LOOP ENDS
    }

    /// Add simple crosshair
    if (render_mode != RenderMode::GLOBAL_ILLUM) {   
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                int idx = ((j + height/2)*width + (i + width/2))*texchannels;
                pixels[idx  ] = 255;
                pixels[idx+1] = 255;
                pixels[idx+2] = 255;
            }
        }
    }

    framenum++;
}

void RayTracer::init(uint32_t _width, uint32_t _height) {
    scene = Scene();
    //model = Scene::noise_model(7, -0.3, 1.0);
    model = Scene::magicaVoxel("../models/temple.vox");

    time_since_tick = 0.0f;
    secs_per_tick = 0.2f;

    env = stbi_loadf("../textures/sunflowers_4k.hdr", &texWidth, &texHeight, &texChannels, 0);
    if (env == NULL){
        printf("Failed to load texture\n");
        exit(1);
    }
    render_mode = RenderMode::SIMPLE;

    device = oidn::newDevice();
    device.commit();

    filter = device.newFilter("RT");

    width = _width;
    height = _height;
    texchannels = 4;
    imageSize = width * height * texchannels;
    fov = 90;
    s = tan(radians(fov/2))*2/width;
    
    states = new prng_state[omp_get_max_threads()];
    for (int i = 0; i < omp_get_max_threads(); i++){
        states[i].state = i+1;
    }

    accumBuffer = new float[width*height*3];
    normalBuffer = new float[width*height*3];
    albedoBuffer = new float[width*height*3];

    eye = model->bounding_box.center()-vec3(0.0,0.0,1.0)*(float)model->bounding_box.z_length();
    vec3 at = model->bounding_box.center();
    vec3 dir = normalize(at-eye);
    vec3 up = cross(cross(dir,vec3(0.0,1.0,0.0)),dir);
    ONB = mat3(cross(dir,up),up,dir); //RIGHT, UP, BACK
    
    std::cout << to_string(up) << std::endl;
    std::cout << determinant(ONB) << std::endl;
    std::cout << glm::to_string(ONB) << std::endl;

    init_controls();
}

void RayTracer::init_controls() {
    int keynums[17];

    keynums[MOVE_FORWARD] = GLFW_KEY_W;
    keynums[MOVE_BACKWARD] = GLFW_KEY_S;
    keynums[MOVE_LEFT] = GLFW_KEY_A;
    keynums[MOVE_RIGHT] = GLFW_KEY_D;
    keynums[MOVE_UPWARDS] = GLFW_KEY_LEFT_SHIFT;
    keynums[MOVE_DOWNWARDS] = GLFW_KEY_LEFT_CONTROL;
    keynums[ROTATE_CCW] = GLFW_KEY_Q;
    keynums[ROTATE_CW] = GLFW_KEY_E;
    keynums[PLACE_BLOCK] = GLFW_KEY_I;
    keynums[REMOVE_BLOCK] = GLFW_KEY_O;
    keynums[RENDER_MODE_SIMPLE] = GLFW_KEY_1;
    keynums[RENDER_MODE_GLOBAL] = GLFW_KEY_2;
    keynums[RENDER_MODE_NORMALS] = GLFW_KEY_3;
    keynums[RENDER_MODE_PIXELIDX] = GLFW_KEY_4;
    keynums[RENDER_MODE_AO] = GLFW_KEY_5;
    keynums[SCREENSHOT] = GLFW_KEY_P;
    keynums[TOGGLEDENOISE] = GLFW_KEY_F;


    for(int i = 0; i < 17; ++i)
        keys[i] = std::pair(keynums[i],GLFW_RELEASE);
}

RayTracer::~RayTracer() {
    delete[] states;
    delete[] accumBuffer;
    delete[] normalBuffer;
    delete[] albedoBuffer;
    stbi_image_free(env);
};