#include "pathTracer.h"
#include <optix_device.h>

inline __device__ float norm2(const owl::vec2f& v) {
    return v.x * v.x + v.y * v.y;
}

OPTIX_RAYGEN_PROGRAM(ptRayGen)()  {
    const RayGenData &self = owl::getProgramData<RayGenData>();
    const owl::vec2i pixelID = owl::getLaunchIndex();

    PerRayData prd;
    prd.random.init(pixelID.x,pixelID.y);
    owl::vec3f colour = 0.f;

    for (int sampleID=0; sampleID < self.samples; sampleID++) {
        owl::Ray ray;

        const owl::vec2f pixelSample(prd.random(),prd.random());
        const owl::vec2f screen
          = (owl::vec2f(pixelID)+pixelSample)
          / owl::vec2f(self.resolution);
        const owl::vec3f origin = self.camera.pos;
        const owl::vec3f direction
            = normalize(self.camera.dir_00
                + screen.u * self.camera.dir_du
                + screen.v * self.camera.dir_dv);

        ray.origin = origin;
        ray.direction = direction;
        traceRay(self.world, ray, prd);

        colour += prd.colour;
    }

    colour = colour * (1.f / self.samples);

    const int fbOfs = pixelID.x+self.resolution.x*pixelID.y;
    self.fbPtr[fbOfs] = owl::make_rgba(colour);
}


OPTIX_MISS_PROGRAM(miss)()
{
    const owl::vec2i pixelID = owl::getLaunchIndex();
    const MissProgData &self = owl::getProgramData<MissProgData>();

    auto &prd = owl::getPRD<PerRayData>();

    owl::vec3f rayDir = optixGetWorldRayDirection();
    rayDir = normalize(rayDir);
    prd.colour = self.sky_colour * (rayDir.y * .5f + 1.f);
}

OPTIX_CLOSEST_HIT_PROGRAM(TriangleMesh)()
{
    auto &prd = owl::getPRD<PerRayData>();

    const TrianglesGeomData &self = owl::getProgramData<TrianglesGeomData>();
    const auto colour = self.material->albedo;
    const int   primID = optixGetPrimitiveIndex();
    const owl::vec3f Ng = normalize(self.normal[primID]);

    const owl::vec3f rayDir = optixGetWorldRayDirection();

    prd.colour = (.2f + .8f*fabs(dot(rayDir,Ng))) * colour;
}