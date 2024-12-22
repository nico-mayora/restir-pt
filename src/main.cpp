#include <iostream>

#include "viewer.h"
#include "loader/mitsuba3.h"


const owl::vec3f init_lookFrom(-4.f,+3.f,-2.f);
const owl::vec3f init_lookAt(0.f,0.f,0.f);
const owl::vec3f init_lookUp(0.f,1.f,0.f);
const float init_cosFovy = 0.66f;

int main()
{
    // TODO: pass scene name as argv
    const auto loader = new Mitsuba3Loader("cornell-box");
    const auto world = loader->load();

    std::cout << world->models.size() << std::endl;
    const auto db_model = world->models.at(3);
    for (const auto &v: db_model->mesh->vertices) {
        std::cout << "vertex: " << v.x << " " << v.y << " " << v.z << '\n';
    }
    std::cout << "vertex count: " << db_model->mesh->vertices.size() << '\n';

    Viewer viewer(world);
    // viewer.camera.setOrientation(init_lookFrom,
    //                          init_lookAt,
    //                          init_lookUp,
    //                          owl::viewer::toDegrees(acosf(init_cosFovy)));

    viewer.enableFlyMode();

    std::cout << "Launching...\n";
    viewer.showAndRun();
}
