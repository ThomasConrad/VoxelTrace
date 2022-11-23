#pragma once
#include <glm/glm.hpp>
#include <glm/fwd.hpp>
#include <math.h>

typedef unsigned int uint;
typedef unsigned char uint8;
struct BBox {
    float x1;
    float x2;
    float y1;
    float y2;
    float z1;
    float z2;

    BBox() : x1(0), x2(0), y1(0), y2(0), z1(0), z2(0) {}
    BBox(float x1, float x2) : x1(x1), x2(x2), y1(x1), y2(x2), z1(x1), z2(x2) {}
    BBox(float x1, float x2, float y1, float y2, float z1, float z2) : x1(x1), x2(x2), y1(y1), y2(y2), z1(z1), z2(z2) {}
    
    glm::vec3 min = glm::vec3(x1, y1, z1);
    glm::vec3 max = glm::vec3(x2, y2, z2);
    glm::vec3 center() {
        return (max-min)/2.0f;
    }
    glm::vec3 size() {
        return max-min;
    }
    bool is_empty() {
        if (x1 == x2 || y1 == y2 || z1 == z2) {
            return true;
        }
        return false;
    }
    uint32_t x_length() {
        return (uint32_t)abs(x2-x1);
    }
    uint32_t y_length() {
        return (uint32_t)abs(y2-y1);
    }
    uint32_t z_length() {
        return (uint32_t)abs(z2-z1);
    }
};

inline float fastInvSqRoot( float n ) {
   
   const float threehalfs = 1.5F;
   float y = n;
   
   long i = * ( long * ) &y;

   i = 0x5f3759df - ( i >> 1 );
   y = * ( float * ) &i;
   
   y = y * ( threehalfs - ( (n * 0.5F) * y * y ) );
   
   return y;
}

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    uint depth;

    //Removing default constructor for now, a ray should *always* have a origin and direction specified
    //Not having so is super undefined
    //Ray() : origin(glm::vec3{ 0.0,0.0,0.0 }), direction(glm::vec3{ 0.0,0.0,0.0 }) {}
    Ray(glm::vec3 origin, glm::vec3 direction) : origin(origin), direction(direction), depth(0) {}
};

struct Voxel {
    glm::vec3 albedo; // Kd
    //int matType; // 0 = diffuse, 1 = metal, 2 = glass, 3 = emissive
    //float emission;
    //float reflectance;
    //float refractance;
    //float ior;
    Voxel() : albedo(glm::vec3(1.0,1.0,1.0)) {};
    Voxel(glm::vec3 col) : albedo(col) {};
    Voxel(glm::vec3 col, int _matType) : albedo(col) {};
    Voxel(glm::vec3 col, float Le) : albedo(col) {};

    //Voxel() : albedo(glm::vec3(1.0,1.0,1.0)), emission(0.0f), matType(0) {}
    //Voxel(glm::vec3 col) : albedo(col), emission(0.0f), matType(0) {}
    //Voxel(glm::vec3 col, float Le) : albedo(col), emission(Le), matType(3) {}
    /*Voxel(glm::vec3 col, int _matType) : albedo(col), matType(_matType) {
        if (matType == 1){
            albedo = glm::vec3(1.0f);
            reflectance = 1.0f;
        }
        else if (matType == 2){
            albedo = (glm::vec3(2.0f)+albedo)/3.0f;
            reflectance = 0.1f;
            refractance = 0.9f;
            ior = 1.3f;
        }
        else if (matType == 3){
            emission = 100.0f;
        }
    }*/


};

struct HitInfo{
    Voxel obj; //voxel hit
    glm::vec3 p; //position
    glm::vec3 n; //normal
    glm::vec3 ray_color; //sampled color at hit position
    bool hit;
    uint depth; //depth of ray
    float t; //distance from ray origin
    bool inside = false; //is ray inside object?

    HitInfo() : obj(Voxel()), p(glm::vec3()), n(glm::vec3(0.0, 0.0, 1.0)), depth(0), hit(false) {};
};