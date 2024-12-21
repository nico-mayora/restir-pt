#include <iostream>

#include "viewer.h"
#include "loader/mitsuba3.h"

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

    viewer.enableFlyMode();

    std::cout << "Launching...\n";
    viewer.showAndRun();
}
