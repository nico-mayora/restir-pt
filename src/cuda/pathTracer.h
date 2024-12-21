#pragma once
#include "owl/APIHandle.h"

enum RayTypes {
    PRIMARY,
    SHADOW,
    RAY_TYPES_COUNT
};

enum MaterialType {
    LAMBERTIAN
};

struct Material {
    MaterialType matType;
    owl::vec3f albedo; // TODO: This might be an image texture in the future.
};

struct TrianglesGeomData {
    Material *material;
    owl::vec3f *vertex;
    owl::vec3i *index;
    owl::vec3f *normal;
    bool faceted;
};

struct DeviceCamera {
    owl::vec3f pos;
    owl::vec3f dir_00;
    owl::vec3f dir_dv;
    owl::vec3f dir_du;
};

struct MissProgData {
    owl::vec3f sky_colour;
};

struct RayGenData {
    uint32_t *fbPtr;
    OptixTraversableHandle world;
    int depth;
    int samples;
    owl::vec2i resolution;
    float fov;

    struct { // we'll just support one light in the xy plane to begin with.
        owl::vec3f centre;
        owl::vec2f sides;
    } lightSource;

    DeviceCamera camera;
};