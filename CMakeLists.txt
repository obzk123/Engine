cmake_minimum_required(VERSION 3.24)

# Detectamos si el engine se compila standalone o incluido desde otro proyecto.
# Si no hay proyecto padre definido, estamos en modo standalone.
set(ENGINE_STANDALONE OFF)
if(NOT DEFINED PROJECT_NAME)
    set(ENGINE_STANDALONE ON)
    project(Engine LANGUAGES C CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# Siempre: compilar la libreria del engine
add_subdirectory(engine)

# Solo standalone: compilar la demo para testear el engine
if(ENGINE_STANDALONE)
    add_subdirectory(demo)
endif()
