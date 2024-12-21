#include "viewer.h"
#include "world.h"
#include "cuda/pathTracer.h"

extern "C" char pathTracer_ptx[];

Viewer::Viewer(const World *world) {
    context = owlContextCreate(nullptr, 1);
    OWLModule module = owlModuleCreate(context, pathTracer_ptx);

    OWLVarDecl triangles_geom_vars[] = {
        { "material", OWL_RAW_POINTER, OWL_OFFSETOF(TrianglesGeomData,material)},
        { "vertex", OWL_BUFPTR, OWL_OFFSETOF(TrianglesGeomData,vertex)},
        { "index",  OWL_BUFPTR, OWL_OFFSETOF(TrianglesGeomData,index)},
        { "normal",  OWL_BUFPTR, OWL_OFFSETOF(TrianglesGeomData,normal)},
        { "faceted", OWL_BOOL, OWL_OFFSETOF(TrianglesGeomData, faceted)},
        { nullptr /* Sentinel to mark end-of-list */}
    };

    OWLGeomType triangles_geom_type
        = owlGeomTypeCreate(context,
                          OWL_TRIANGLES,
                          sizeof(TrianglesGeomData),
                          triangles_geom_vars,-1);
    owlGeomTypeSetClosestHit(triangles_geom_type,PRIMARY,
                             module,"TriangleMesh");

    std::cout << "Building geometries...\n";

    // Upload meshes to GPU
    std::vector<OWLGeom> geometries;
    for (const auto model : world->models) {
        const auto mesh = model->mesh;
        OWLBuffer vertex_buffer
            = owlDeviceBufferCreate(context, OWL_FLOAT3, mesh->vertices.size(), mesh->vertices.data());
        OWLBuffer normal_buffer
            = owlDeviceBufferCreate(context, OWL_FLOAT3, mesh->normals.size(), mesh->normals.data());
        OWLBuffer index_buffer
            = owlDeviceBufferCreate(context,OWL_INT3,mesh->indices.size(), mesh->indices.data());

        OWLGeom triangles_geom
            = owlGeomCreate(context, triangles_geom_type);

        owlTrianglesSetVertices(triangles_geom,vertex_buffer,
                                mesh->vertices.size(),sizeof(owl::vec3f),0);
        owlTrianglesSetIndices(triangles_geom,index_buffer,
                               mesh->indices.size(),sizeof(owl::vec3i),0);

        owlGeomSetBuffer(triangles_geom,"vertex", vertex_buffer);
        owlGeomSetBuffer(triangles_geom,"index", index_buffer);
        owlGeomSetBuffer(triangles_geom,"normal", normal_buffer);
        owlGeomSet1b(triangles_geom, "faceted", mesh->faceted);

        // Copy material to device memory.
        Material *mat_ptr;
        cudaMalloc(reinterpret_cast<void**>(&mat_ptr),sizeof(Material));
        cudaMemcpy(mat_ptr, model->material, sizeof(Material), cudaMemcpyHostToDevice);
        owlGeomSetPointer(triangles_geom, "material", mat_ptr);

        geometries.emplace_back(triangles_geom);
    }

    OWLGroup triangles_group
        = owlTrianglesGeomGroupCreate(context,geometries.size(),geometries.data());
    owlGroupBuildAccel(triangles_group);
    OWLGroup owl_world
        = owlInstanceGroupCreate(context,1);
    owlInstanceGroupSetChild(owl_world,0,triangles_group);
    owlGroupBuildAccel(owl_world);

    // TODO: Load lights

    // Miss program
    OWLVarDecl missProgVars[] =
    {
        { "sky_colour", OWL_FLOAT3, OWL_OFFSETOF(MissProgData, sky_colour)},
        { /* sentinel to mark end of list */ }
    };
    OWLMissProg missProg
      = owlMissProgCreate(context,module,"miss",sizeof(MissProgData),
                          missProgVars,-1);
    owlMissProgSet3f(missProg,"sky_colour",owl3f{.1f,.3f,.5f});

    OWLVarDecl rayGenVars[] = {
        { "fbPtr",         OWL_RAW_POINTER, OWL_OFFSETOF(RayGenData,fbPtr)},
        { "depth", OWL_FLOAT3, OWL_OFFSETOF(RayGenData,depth)},
        { "samples", OWL_FLOAT3, OWL_OFFSETOF(RayGenData,samples)},
        { "resolution", OWL_INT2, OWL_OFFSETOF(RayGenData,resolution)},
        { "fov", OWL_FLOAT3, OWL_OFFSETOF(RayGenData,fov)},
        { "world",         OWL_GROUP,  OWL_OFFSETOF(RayGenData,world)},
        { "camera.pos",    OWL_FLOAT3, OWL_OFFSETOF(RayGenData,camera.pos)},
        { "camera.dir_00", OWL_FLOAT3, OWL_OFFSETOF(RayGenData,camera.dir_00)},
        { "camera.dir_dv", OWL_FLOAT3, OWL_OFFSETOF(RayGenData,camera.dir_dv)},
        { "camera.dir_du", OWL_FLOAT3, OWL_OFFSETOF(RayGenData,camera.dir_du)},
        { "lightSource.centre", OWL_FLOAT3, OWL_OFFSETOF(RayGenData,lightSource.centre)},
        { "lightSource.sides", OWL_FLOAT3, OWL_OFFSETOF(RayGenData,lightSource.sides)},
        { /* sentinel to mark end of list */ }
    };

    rayGen
        = owlRayGenCreate(context,module,"ptRayGen", sizeof(RayGenData), rayGenVars,-1);
    owlRayGenSetGroup(rayGen,"world", owl_world);

    // Initialise Viewer camera with params from scene description.
    camera.setOrientation(world->cam->lookFrom,
                          world->cam->lookAt,
                          world->cam->up,
                          world->cam->image.fov,
                          false);

    owlBuildPrograms(context);
    owlBuildPipeline(context);
    owlBuildSBT(context);
}

void Viewer::render()
{
    if (sbtDirty) {
        owlBuildSBT(context);
        sbtDirty = false;
    }
    owlRayGenLaunch2D(rayGen,fbSize.x,fbSize.y);
}

void Viewer::resize(const owl::vec2i &newSize)
{
    OWLViewer::resize(newSize);
    cameraChanged();
}

void Viewer::cameraChanged()
{
    const owl::vec3f lookFrom = camera.getFrom();
    const owl::vec3f lookAt = camera.getAt();
    const owl::vec3f lookUp = camera.getUp();
    const float cosFovy = camera.getCosFovy();
    // ----------- compute variable values  ------------------
    owl::vec3f camera_pos = lookFrom;
    owl::vec3f camera_d00
      = normalize(lookAt-lookFrom);
    float aspect = fbSize.x / float(fbSize.y);
    owl::vec3f camera_ddu
      = cosFovy * aspect * normalize(cross(camera_d00,lookUp));
    owl::vec3f camera_ddv
      = cosFovy * normalize(cross(camera_ddu,camera_d00));
    camera_d00 -= 0.5f * camera_ddu;
    camera_d00 -= 0.5f * camera_ddv;

    // ----------- set variables  ----------------------------
    owlRayGenSet1ul   (rayGen,"fbPtr",        reinterpret_cast<uint64_t>(fbPointer));
    owlRayGenSet2i    (rayGen,"resolution",   reinterpret_cast<const owl2i&>(fbSize));
    owlRayGenSet3f    (rayGen,"camera.pos",   reinterpret_cast<const owl3f&>(camera_pos));
    owlRayGenSet3f    (rayGen,"camera.dir_00",reinterpret_cast<const owl3f&>(camera_d00));
    owlRayGenSet3f    (rayGen,"camera.dir_du",reinterpret_cast<const owl3f&>(camera_ddu));
    owlRayGenSet3f    (rayGen,"camera.dir_dv",reinterpret_cast<const owl3f&>(camera_ddv));
    sbtDirty = true;
}