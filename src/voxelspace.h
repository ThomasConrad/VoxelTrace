#pragma once

#include "octree.h"
#include "helpers.hpp"
#include <iostream>
#include <cmath>

template <typename T>
class VoxelSpace {
    uint range;

    glm::vec3 world_to_octree;
    glm::vec3 octree_to_world;

    //Is a point in the bounding box?
    bool is_point_in_bounding_box(glm::vec3 const& point) {
        float x = point.x; float y = point.y; float z = point.z;
        if (x < bounding_box.x1 || x >= bounding_box.x2) return false;
        if (y < bounding_box.y1 || y >= bounding_box.y2) return false;
        if (z < bounding_box.z1 || z >= bounding_box.z2) return false;
        return true;
    }

    //Translates a floating point in space to the corresponding index of a (deepest level) node in the octree
    glm::uvec3 point_to_octree_index(glm::vec3 const& point) {
		//as it's all uints, casting should just floor it
        return (glm::uvec3)((point-bounding_box.min)*world_to_octree);
    }
    //Translates index in the octree to corresponding position in space (of corner closest to origin)
    glm::vec3 octree_index_to_point(glm::uvec3 const& oct_index) {
        return glm::vec3(oct_index)*octree_to_world + bounding_box.min;
    }

    //For bbox calculation
    static uint bound_start(uint i, uint c) {
        return (i >> c) << c;
    }
    
    static uint bound_stop(uint i, uint c) {
        return ((i >> c) + 1) << c;
    }
    //Computes the largest bounding box (node) in the octree which contains the point, and is also guaranteed to be empty
    //Assumes that the given oct_index is within the octee
    BBox largest_safe_bbox(glm::uvec3 const& oct_index, uint stop_depth) {
        uint x = oct_index.x; uint y = oct_index.y; uint z = oct_index.z;
        uint c = octree.max_depth - stop_depth;

        glm::vec3 start = octree_index_to_point(glm::uvec3{ bound_start(x, c), bound_start(y, c), bound_start(z, c) });
        glm::vec3 stop = octree_index_to_point(glm::uvec3{ bound_stop(x, c), bound_stop(y, c), bound_stop(z, c) });

        return BBox{
            start.x, stop.x,
            start.y, stop.y,
            start.z, stop.z,
        };
    }

public:
    OcTree<T> octree;
    BBox bounding_box; //Points in the voxel space are greater or equal to the lower bound, and strictly lower than the upper bound

    //Test if a point is in the voxel space, and if so, output the octree index for it
    bool try_point_to_octree_index(glm::vec3 const& point, glm::uvec3& out) {
        if (is_point_in_bounding_box(point)) {
            out = point_to_octree_index(point);
            return true;
        }
        return false;
    }
    VoxelSpace(BBox bounding_box, uint depth)
        : bounding_box(bounding_box), octree(OcTree<T>(depth)) {
        range = octree.get_range();
        float w_x = bounding_box.x2 - bounding_box.x1;
        float w_y = bounding_box.y2 - bounding_box.y1;
        float w_z = bounding_box.z2 - bounding_box.z1;
        world_to_octree = glm::vec3{
            (float)range/w_x,
            (float)range/w_y,
            (float)range/w_z
        };
        octree_to_world = glm::vec3{
            w_x / (float)range,
            w_y / (float)range,
            w_z / (float)range
        };
    }

    uint get_range() {
        return range;
    }

    bool try_get_voxel_at_point(glm::vec3 const& point, T& out) {
        glm::uvec3 oct_index;
        if (try_point_to_octree_index(point, oct_index)) {
            OcTreeResult<T> result;
            bool voxel_exists = octree.get(oct_index.x, oct_index.y, oct_index.z, result);
            if (voxel_exists) {
                out = result.item;
                return true;
            }
        }
        return false;
    }

    bool place_voxel_at_point(glm::vec3 const& point, T item) {
        glm::uvec3 oct_index;
        if (try_point_to_octree_index(point, oct_index)) {
            octree.insert(oct_index.x, oct_index.y, oct_index.z, item);
            return true;
        }
        return false;
    }
    bool remove_voxel_at_point(glm::vec3 const& point) {
        glm::uvec3 oct_index;
        if (try_point_to_octree_index(point, oct_index)) {
            octree.remove(oct_index.x, oct_index.y, oct_index.z);
            return true;
        }
        return false;
    }

    bool trace_aabb(Ray r, float &t,BBox aabb, bool getMax = false) {
        glm::vec3 invdir = 1.0f / r.direction;
        glm::vec3 t0 = (aabb.min - r.origin) * invdir;
        glm::vec3 t1 = (aabb.max - r.origin) * invdir;

        glm::vec3 tmin = min(t0, t1);
        glm::vec3 tmax = max(t0, t1);
        if (getMax) {
            glm::vec3 tmp = tmin;
            tmin = tmax;
            tmax = tmp;
        };
        t = fmax(tmin.x, fmax(tmin.y, tmin.z));
        float tmax_ = fmin(tmax.x, fmin(tmax.y, tmax.z));
        
        return (t < tmax_ && t > 0);
    }


    bool intersect(Ray const& ray, HitInfo &out){        
        float eps = 1e-4f;
        Ray r = ray;
        out.depth = 0;
        out.t = 0;
        //Initial hit attempt
        glm::uvec3 oct_index;
        bool inside = false;
        bool started_outside = !try_point_to_octree_index(r.origin, oct_index);
        if (started_outside){
            float t;
            if (!trace_aabb(r,t,bounding_box)) return false;
            r.origin += r.direction * (t + eps);
            if (!try_point_to_octree_index(r.origin, oct_index)) return false;
        }
        

        OcTreeResult<T> result;
        bool voxel_hit = octree.get(oct_index.x, oct_index.y, oct_index.z, result);
        if (!started_outside){
            inside = voxel_hit;
            voxel_hit ^= inside;
        }
        out.inside = inside;
        float sign_dir_x = r.direction.x < 0 ? -1.0 : 1.0;
        float sign_dir_y = r.direction.y < 0 ? -1.0 : 1.0;
        float sign_dir_z = r.direction.z < 0 ? -1.0 : 1.0;
        
        //We really don't want to divide by zero
        if (std::fabs(r.direction.x) < eps) r.direction.x = eps*sign_dir_x;
        if (std::fabs(r.direction.y) < eps) r.direction.y = eps*sign_dir_y;
        if (std::fabs(r.direction.z) < eps) r.direction.z = eps*sign_dir_z;
        
        //precalculate inverse direction
        double inv_dir_x = 1.0f / r.direction.x;
        double inv_dir_y = 1.0f / r.direction.y;
        double inv_dir_z = 1.0f / r.direction.z;

        glm::vec3 out_normal = glm::vec3(0.f);

        //While nothing is hit
        while (!voxel_hit && out.depth < octree.max_depth*octree.max_depth*octree.max_depth) {
            BBox bbox = largest_safe_bbox(oct_index, result.depth);

            //Target x, y and z values given the ray direction and bbox
            float x = sign_dir_x == -1.0 ? bbox.x1 : bbox.x2;
            float y = sign_dir_y == -1.0 ? bbox.y1 : bbox.y2;
            float z = sign_dir_z == -1.0 ? bbox.z1 : bbox.z2;

            //Calculate t to exit bbox for each cardinal direction
            double tx = (x - r.origin.x) * inv_dir_x;
            double ty = (y - r.origin.y) * inv_dir_y;
            double tz = (z - r.origin.z) * inv_dir_z;
           
            //Pick smallest t to hit the next bbox
            float t; 
            if (tx < ty) {
                t = tx; out_normal = glm::vec3(-sign_dir_x, 0.0, 0.0);
            } else {
                t = ty; out_normal = glm::vec3(0.0, -sign_dir_y, 0.0);
            }
            if (tz < t) {
                t = tz; out_normal = glm::vec3(0.0, 0.0, -sign_dir_z);
            }
            if (t < eps) return false;
            //Advance ray
            r.origin = r.origin + r.direction * t - out_normal * 5e-5f;
            out.t += t;
            out.depth++;

            //Try to hit again
            if (!try_point_to_octree_index(r.origin, oct_index)) return false; //If the ray has exited the voxel space, there was no hit
            voxel_hit = octree.get(oct_index.x, oct_index.y, oct_index.z, result);
            if (voxel_hit && inside && result.item.matType != 2) {
                out.obj = result.item;
                out.n = out_normal;
                out.p = r.origin;
                break;
            }
            voxel_hit ^= inside;
        }

        if (voxel_hit) {
            out.obj = result.item;
            out.p = r.origin;
            out.n = out_normal;
        }
        return voxel_hit;
    }
};