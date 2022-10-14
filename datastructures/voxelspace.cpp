#include "octree.h"
#include "3d_prims.h"
#include <iostream>
#include <tuple>
#include <cmath>

const uint DEPTH = 2;

template <typename T>
struct VoxelHit {
    bool hit;
    T voxel;

    VoxelHit() : hit(false), voxel(T()) {}
};

template <typename T>
class VoxelSpace {
    OcTree<T> octree;
    uint range;
    BBox bounding_box; //Points in the voxel space are greater or equal to the lower bound, and strictly lower than the upper bound

    Vec3 world_to_octree;
    Vec3 octree_to_world;

    //Is a point in the bounding box?
    bool is_point_in_bounding_box(Vec3 const& point) {
        double x = point.x; double y = point.y; double z = point.z;
        if (x < bounding_box.x1 || x >= bounding_box.x2) return false;
        if (y < bounding_box.y1 || y >= bounding_box.y2) return false;
        if (z < bounding_box.z1 || z >= bounding_box.z2) return false;
        return true;
    }

    //Test if a point is in the voxel space, and if so, output the octree index for it
    bool try_point_to_octree_index(Vec3 const& point, UVec3& out) {
        if (is_point_in_bounding_box(point)) {
            out = point_to_octree_index(point);
            return true;
        }
        return false;
    }

    //Translates a floating point in space to the corresponding index of a (deepest level) node in the octree
    UVec3 point_to_octree_index(Vec3 const& point) {
        double x = point.x; double y = point.y; double z = point.z;
        uint i_x = std::floor( (x-bounding_box.x1)*world_to_octree.x );
        uint i_y = std::floor( (y-bounding_box.y1)*world_to_octree.y );
        uint i_z = std::floor( (z-bounding_box.z1)*world_to_octree.z );

        return UVec3{i_x,i_y,i_z};
    }
    //Translates index in the octree to corresponding position in space (of corner closest to origin)
    Vec3 octree_index_to_point(UVec3 const& oct_index) {
        double i_x = (double)oct_index.x; double i_y = (double)oct_index.y; double i_z = (double)oct_index.z;
        double x = i_x * octree_to_world.x + bounding_box.x1;
        double y = i_y * octree_to_world.y + bounding_box.y1;
        double z = i_z * octree_to_world.z + bounding_box.z1;

        return Vec3{x,y,z};
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
    BBox largest_safe_bbox(UVec3 const& oct_index, uint stop_depth) {
        uint x = oct_index.x; uint y = oct_index.y; uint z = oct_index.z;
        uint c = octree.max_depth - stop_depth;

        Vec3 start = octree_index_to_point(UVec3{ bound_start(x, c), bound_start(y, c), bound_start(z, c) });
        Vec3 stop = octree_index_to_point(UVec3{ bound_stop(x, c), bound_stop(y, c), bound_stop(z, c) });

        return BBox{
            start.x, stop.x,
            start.y, stop.y,
            start.z, stop.z,
        };
    }

public:

    VoxelSpace<T>(BBox bounding_box) : bounding_box(bounding_box), octree(OcTree<T>(DEPTH)) {
        range = octree.get_range();
        double w_x = bounding_box.x2 - bounding_box.x1;
        double w_y = bounding_box.y2 - bounding_box.y1;
        double w_z = bounding_box.z2 - bounding_box.z1;
        world_to_octree = Vec3{
            (double)range/w_x,
            (double)range/w_y,
            (double)range/w_z
        };
        octree_to_world = Vec3{
            w_x / (double)range,
            w_y / (double)range,
            w_z / (double)range
        };
    }

    uint get_range() {
        return range;
    }

    bool try_get_voxel_at_point(Vec3 const& point, T& out) {
        UVec3 oct_index;
        if (try_point_to_octree_index(point, oct_index)) {
            auto result = octree.get(oct_index.x, oct_index.y, oct_index.z);
            if (std::get<0>(result)) {
                out = std::get<2>(result);
                return true;
            }
        }
        return false;
    }

    bool place_voxel_at_point(Vec3 const& point, T item) {
        UVec3 oct_index;
        if (try_point_to_octree_index(point, oct_index)) {
            octree.insert(oct_index.x, oct_index.y, oct_index.z, item);
            return true;
        }
        return false;
    }

    VoxelHit<T> intersect(Ray const& ray) {
        double eps = 1e-8;
        Ray r = ray;
        VoxelHit<T> hit_record = VoxelHit<T>();

        //std::cout << "ray at (" << r.origin.x << ", " << r.origin.y << ", " << r.origin.z << ")\n";

        //Initial hit attempt
        UVec3 oct_index;
        if (!try_point_to_octree_index(r.origin, oct_index)) return hit_record; //TODO: For now, the ray must start within the voxel space, this shouldn't be the case
        auto result = octree.get(oct_index.x, oct_index.y, oct_index.z);

        //While nothing is hit
        while (!std::get<0>(result)) {
            BBox bbox = largest_safe_bbox(oct_index, std::get<1>(result));

            //Target x, y and z values given the ray direction and bbox
            double x = r.direction.x < 0 ? bbox.x1 : bbox.x2;
            double y = r.direction.y < 0 ? bbox.y1 : bbox.y2;
            double z = r.direction.z < 0 ? bbox.z1 : bbox.z2;

            //Calculate t to exit bbox for each cardinal direction
            double tx = (x - r.origin.x) / r.direction.x;
            double ty = (y - r.origin.y) / r.direction.y;
            double tz = (z - r.origin.z) / r.direction.z;

            //Pick smallest t to hit the next bbox
            double t = (tx < ty) ? tx : ty;
            t = (t < tz) ? t : tz;

            //Advance ray
            r.origin = r.origin + r.direction * (t + eps);
            //std::cout << "ray at (" << r.origin.x << ", " << r.origin.y << ", " << r.origin.z << ")\n";

            //Try to hit again
            if (!try_point_to_octree_index(r.origin, oct_index)) return hit_record; //If the ray has exited the voxel space, there was no hit
            result = octree.get(oct_index.x, oct_index.y, oct_index.z);
        }
        //If the loop is exited, something was hit
        hit_record.hit = true;
        hit_record.voxel = std::get<2>(result);
        return hit_record;
    }
};

/*
int main() {
    VoxelSpace<int> model = VoxelSpace<int>(BBox{0.0, 8.0, 0.0, 8.0, 0.0, 8.0});
    model.place_voxel_at_point(Vec3{0.5, 1.5, 0.0}, 1337);
    int out;
    model.try_get_voxel_at_point(Vec3{0.5, 1.7, 0.5}, out);
    std::cout << "Place-get: Should be 1337: " << out << "\n";
    Ray r = Ray(Vec3{ 2.5, 4.5, 0.0 }, Vec3{ -1.0, -2.0, 0.0 });
    std::cout << "Intersection: Should return 1: " << model.intersect(r).hit;

    return 0;
}
*/