#pragma once

typedef unsigned int uint;

struct BBox {
    double x1;
    double x2;
    double y1;
    double y2;
    double z1;
    double z2;

    bool is_empty() {
        if (x1 == x2 || y1 == y2 || z1 == z2) {
            return true;
        }
        return false;
    }
};

struct Vec3 {
    double x;
    double y;
    double z;

    Vec3 operator*(double t) {
        return Vec3{x*t,y*t,z*t};
    }
    Vec3 operator+(Vec3 other) {
        return Vec3{ x + other.x, y + other.y, z + other.z };
    }
};
struct UVec3 {
    uint x;
    uint y;
    uint z;
};

struct Ray {
    Vec3 origin;
    Vec3 direction;

    Ray() : origin(Vec3{ 0.0,0.0,0.0 }), direction(Vec3{ 0.0,0.0,0.0 }) {}
    Ray(Vec3 origin, Vec3 direction) : origin(origin), direction(direction) {}
};