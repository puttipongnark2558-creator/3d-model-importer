cmake_minimum_required(VERSION 3.25)
project(3d-model-importer LANGUAGES CXX C)

find_package(Geode REQUIRED)

add_geode_mod(3d-model-importer
    src/main.cpp
)

target_link_libraries(3d-model-importer PRIVATE geode::geode)
