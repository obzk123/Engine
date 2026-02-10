# ENGINE_ROOT apunta a la raiz del repo del engine, independientemente
# de si se compila standalone o incluido desde otro proyecto.
# CMAKE_CURRENT_SOURCE_DIR = engine-repo/engine/
# Subimos un nivel para llegar a engine-repo/
set(ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_library(engine STATIC
    src/Engine.cpp
    src/Time.cpp
    src/Input.cpp
    src/ecs/Registry.cpp
    src/ecs/DemoWorld.cpp
    src/ecs/SystemScheduler.cpp
    src/ecs/systems/InputSystem.cpp
    src/ecs/systems/PlayerControlSystem.cpp
    src/ecs/systems/RenderSystem.cpp
    src/render/Renderer2D.cpp

    # ImGui core (vendorizado)
    ${ENGINE_ROOT}/external/imgui/imgui.cpp
    ${ENGINE_ROOT}/external/imgui/imgui_draw.cpp
    ${ENGINE_ROOT}/external/imgui/imgui_tables.cpp
    ${ENGINE_ROOT}/external/imgui/imgui_widgets.cpp
    # opcional: demo (util para testear)
    ${ENGINE_ROOT}/external/imgui/imgui_demo.cpp

    # ImGui backends oficiales (mismo repo/version)
    ${ENGINE_ROOT}/external/imgui/backends/imgui_impl_sdl2.cpp
    ${ENGINE_ROOT}/external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Includes de ImGui (PRIVATE: solo el engine los necesita internamente)
target_include_directories(engine PRIVATE
    ${ENGINE_ROOT}/external/imgui
    ${ENGINE_ROOT}/external/imgui/backends
)

find_package(SDL2 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# stb header-only
find_path(STB_INCLUDE_DIR "stb_image.h")
if (NOT STB_INCLUDE_DIR)
    message(FATAL_ERROR "stb headers not found (stb_image.h). Check vcpkg install.")
endif()
target_include_directories(engine PUBLIC ${STB_INCLUDE_DIR})

target_link_libraries(engine PUBLIC
    SDL2::SDL2
    SDL2::SDL2main
    glad::glad
    glm::glm
)

if (MSVC)
    target_compile_options(engine PRIVATE /W4 /permissive-)
else()
    target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
