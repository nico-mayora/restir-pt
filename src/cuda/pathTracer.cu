#include "pathTracer.h"
#include <optix_device.h>

inline __device__ float norm2(const owl::vec2f& v) {
    return v.x * v.x + v.y * v.y;
}

OPTIX_RAYGEN_PROGRAM(ptRayGen)()  {
    const RayGenData &self = owl::getProgramData<RayGenData>();
    const owl::vec2i pixelID = owl::getLaunchIndex();

    const owl::vec2f screen = (owl::vec2f(pixelID)+owl::vec2f(.5f)) / owl::vec2f(self.resolution);
    owl::Ray ray;
    ray.origin = self.camera.pos;
    ray.direction
      = normalize(self.camera.dir_00
                  + screen.u * self.camera.dir_du
                  + screen.v * self.camera.dir_dv);
    if (pixelID == owl::vec2i(600, 400)) {
        printf("----------\npixelID: %d %d\n", pixelID.x, pixelID.y);
        printf("ray org: %f %f %f, ray dir: %f %f %f\n----------\n", ray.origin.x, ray.origin.y, ray.origin.z, ray.direction.x, ray.direction.y, ray.direction.z);
    }

    owl::vec3f color;
    traceRay(self.world, ray, color);

    const int fbOfs = pixelID.x+self.resolution.x*pixelID.y;
    self.fbPtr[fbOfs] = owl::make_rgba(color);
}


OPTIX_MISS_PROGRAM(miss)()
{
    const owl::vec2i pixelID = owl::getLaunchIndex();
    const MissProgData &self = owl::getProgramData<MissProgData>();

    owl::vec3f &prd = owl::getPRD<owl::vec3f>();
    prd = self.sky_colour;
}

OPTIX_CLOSEST_HIT_PROGRAM(TriangleMesh)()
{
    owl::vec3f &prd = owl::getPRD<owl::vec3f>();

    const TrianglesGeomData &self = owl::getProgramData<TrianglesGeomData>();
    const auto colour = self.material->albedo;
    const int   primID = optixGetPrimitiveIndex();
    const owl::vec3f Ng = normalize(self.normal[primID]);

    const owl::vec3f rayDir = optixGetWorldRayDirection();
    const owl::vec3f rayOrg = optixGetWorldRayOrigin();
    const auto tmax = optixGetRayTmax();
    const owl::vec3f hitpoint = rayOrg + tmax * rayDir;
    if (abs(hitpoint.x - 1.f) < 0.1) {
        prd = owl::vec3f(1.f, 0.f, 0.f);
        return;
    }


    prd = (.2f + .8f*fabs(dot(rayDir,Ng)))*colour;
}