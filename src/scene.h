#pragma once
#include "helpers.hpp"
#include "voxelspace.h"



struct Scene {
    glm::vec3 directional_light_dir; //Directional light
    glm::vec3 directional_light_col;
    float directional_light;
    glm::vec3 ambient_light_col;
    float ambient_light;

    Scene() :
    directional_light_dir   (glm::normalize(glm::vec3(-2,18,1))),
    directional_light_col   (glm::vec3(255, 197, 143)/255.0f), //evening sun, 2600K
    directional_light       (20.0f),
    ambient_light_col       (glm::vec3(1,1,1)),
    ambient_light           (0.f){}

    //3d perlin noise model
    static VoxelSpace<Voxel>* noise_model(uint levels, float threshold, float scale);
    
    //Load from a magicavoxel file
    static VoxelSpace<Voxel>* magicaVoxel(std::string file_name);
    
    //Custom model set in scene source
    static VoxelSpace<Voxel>* custom();
    
    //Passes through one of the other shaders
    static VoxelSpace<Voxel>* passthrough();
};

