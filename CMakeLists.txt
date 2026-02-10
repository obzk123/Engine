cmake_minimum_required(VERSION 3.24)

# Si el engine se compila standalone (no como subdirectorio de otro proyecto),
# creamos el proyecto aca. Si lo incluye un proyecto padre, esto se saltea.
if(NOT DEFINED PROJECT_NAME)
    project(Engine LANGUAGES C CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_subdirectory(engine)
