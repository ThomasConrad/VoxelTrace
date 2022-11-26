#include "scene.h"
#include "PerlinNoise.hpp"
#include <vector>
#define OGT_VOX_IMPLEMENTATION
#include <ogt_vox.h>
#include <stdio.h>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "utility"

VoxelSpace<Voxel>* Scene::noise_model(uint levels, float threshold, float scale) {
    uint size = 1 << levels;
    VoxelSpace<Voxel>* model = new VoxelSpace<Voxel>(BBox{0.0f, (float)size, 0.0f, (float)size, 0.0f, (float)size}, levels - 1);
    const siv::PerlinNoise::seed_type seed = rand();
	const siv::PerlinNoise perlin{seed};
    for (float i = 0; i < size; i++) {
        for (float j = 0; j < size; j++) {
            for (float k = 0; k < size; k++) {
                if (perlin.noise3D(i*scale/size,j*scale/size,k*scale/size) > threshold){
                    uint rand_adjust_red = rand()%30 - 15;
                    uint rand_adjust_gb = rand()%20 - 10;
                    model->place_voxel_at_point(glm::vec3(i,j,k),Voxel(
                        glm::vec3(
                            220 + rand_adjust_red,
                            60 + rand_adjust_gb,
                            60 + rand_adjust_gb
                        )/255.0f
                    ));
                }
            }
        }
    }
    std::cout << "Created noise model\n";
    return model;
}

VoxelSpace<Voxel>* Scene::magicaVoxel(std::string file_name) {
    //load file into buffer
    FILE* fp;
    errno_t err;
   
    if ((err = fopen_s(&fp, file_name.c_str(), "r")) != 0){
        throw std::runtime_error("cannot open file!");
    }
    fseek(fp, 0, SEEK_END);
    const uint32_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint8_t* buffer = new uint8_t[size];
    fread(buffer, size, 1, fp);
    fclose(fp);
    //create voxelscene and destroy buffer
    const ogt_vox_scene* scene = ogt_vox_read_scene(buffer, size);

    delete[] buffer;
    std::vector<std::pair<glm::vec3,Voxel>> voxels;

    for (uint i = 0; i < scene->num_instances; i++) {
        uint32_t model_index = scene->instances[i].model_index;
        ogt_vox_transform mat = scene->instances[model_index].transform;
        glm::mat4 transform = glm::make_mat4(reinterpret_cast<float*>(&mat));
        const ogt_vox_model* model = scene->models[model_index];

        for (uint i = 0; i < model->size_x; i++) {
        for (uint j = 0; j < model->size_y; j++) {
        for (uint k = 0; k < model->size_z; k++) {
            uint32_t voxel_index = i + (j * model->size_x) + (k * model->size_x * model->size_y);
            uint8_t color_index = model->voxel_data[voxel_index];

            if (color_index != 0) {
                ogt_vox_rgba color = scene->palette.color[ color_index];
                uint type = scene->materials.matl[color_index].type;

                glm::vec3 c(color.r, color.g, color.b);
                c /= 255.0f;
                glm::vec3 loc = glm::vec3(transform*glm::vec4{i,j,k,1.0f});
                loc = glm::vec3(loc.x, loc.z, loc.y);
                if (type == 3) //emmisive material
                    voxels.push_back(std::make_pair(loc,Voxel(c,100.0f))); //Swap because of different coordinate system
                else if (type == 2) //transparent material
                    voxels.push_back(std::make_pair(loc,Voxel(c,(int)2)));
                else if (type == 1) //metallic material
                    voxels.push_back(std::make_pair(loc,Voxel(c,(int)1)));
                else
                    voxels.push_back(std::make_pair(loc,Voxel(c)));
            }
        }
        }
        }
    }
    ogt_vox_destroy_scene(scene);

    auto xExtremes = std::minmax_element(voxels.begin(), voxels.end(),
                                     [](const std::pair<glm::vec3,Voxel>& lhs, const std::pair<glm::vec3,Voxel>& rhs) {
                                        return lhs.first.x < rhs.first.x;
                                     });
    auto yExtremes = std::minmax_element(voxels.begin(), voxels.end(),
                                        [](const std::pair<glm::vec3,Voxel>& lhs, const std::pair<glm::vec3,Voxel>& rhs) {
                                            return lhs.first.y < rhs.first.y;
                                        });
    auto zExtremes = std::minmax_element(voxels.begin(), voxels.end(),
                                        [](const std::pair<glm::vec3,Voxel>& lhs, const std::pair<glm::vec3,Voxel>& rhs) {
                                            return lhs.first.z < rhs.first.z;
                                        });

    BBox bbox{xExtremes.first->first.x, xExtremes.second->first.x,
              yExtremes.first->first.y, yExtremes.second->first.y,
              zExtremes.first->first.z, zExtremes.second->first.z};
    uint length = std::max(std::max(bbox.x_length(), bbox.y_length()), bbox.z_length());

    //https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    length--;
    length |= length >> 1;
    length |= length >> 2;
    length |= length >> 4;
    length |= length >> 8;
    length |= length >> 16;
    length++;

    bbox = BBox{bbox.x1, bbox.x1+length,
                bbox.y1, bbox.y1+length,
                bbox.z1, bbox.z1+length};

    auto vSpace = new VoxelSpace<Voxel>(bbox, (uint)floor(std::log2(length))-1);
    for (auto& v : voxels) 
        vSpace->place_voxel_at_point(v.first, v.second);

    return vSpace;
}