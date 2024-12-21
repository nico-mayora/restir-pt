#pragma once
#include <vector>

#include "owl/common/math/vec.h"
#include "pt-math.h"
#include "cuda/pathTracer.h"

struct Mesh {
    std::vector<owl::vec3i> indices;
    std::vector<owl::vec3f> vertices;
    std::vector<owl::vec3f> normals;
    bool faceted = true; // True -> face normals. False -> vertex normals.

    static Mesh *makeBaseRectangle() {
        const auto mesh = new Mesh;
        mesh->vertices = {{1,1,0}, {-1,-1,0}, {1,-1,0}, {-1,1,0}};
        mesh->indices = {{0,2,3}, {1, 3, 2}};
        mesh->normals = {{0,0,1}, {0,0,1}};
        mesh->faceted = true;
        return mesh;
    }

    static Mesh *makeBaseCube() {
        const auto mesh = new Mesh;
        mesh->vertices =
            { {1,1,-1}, {-1,-1,-1}, {1,-1,-1}, {-1,1,-1}
            , {1,1,1 }, {-1,-1,1 }, {1,-1,1 }, {-1,1,1 }
            };

        mesh->indices = {
            {0,4,7}, {0,7,3},
            {2,5,1}, {2,6,5},
            {0,2,4}, {2,6,4},
            {1,7,3}, {1,5,7},
            {4,6,7}, {6,5,7},
            {0,1,2}, {0,3,1}
        };

        mesh->faceted = true;
        for (const auto tri: mesh->indices) {
            owl::vec3f v1 = mesh->vertices.at(tri.z) - mesh->vertices.at(tri.x);
            owl::vec3f v2 = mesh->vertices.at(tri.y) - mesh->vertices.at(tri.x);
            auto normal = normalize(cross(v1, v2));
            mesh->normals.emplace_back(normal);
        }

        return mesh;
    }

    void applyTransform(const Mat4f& tf) {
        for (auto &v: vertices) {
            auto transformed_vtx = tf * owl::vec4f(v, 1);
            v = owl::vec3f(transformed_vtx);
        }

        for (auto &n: normals) {
            auto transformed_vtx = tf * owl::vec4f(n, 0);
            n = owl::vec3f(transformed_vtx);
        }
    }
};

struct Model {
    Mesh *mesh;
    Material *material;
};

struct LightSource {
    Mesh *mesh;
    owl::vec3f radiance;
};

struct Camera {
    owl::vec3f lookFrom;
    owl::vec3f lookAt;
    owl::vec3f up;

    struct {
        int depth;
        int samples;
        owl::vec2i resolution;
        float fov;
    } image;
};

struct World {
    std::vector<Model*> models;
    std::vector<LightSource*> light_sources;

    Camera *cam;
};