cmake_minimum_required(VERSION 3.25)
project(3dmodelimporter VERSION 1.0.0)

find_package(Geode REQUIRED)
setup_geode_mod(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
)
