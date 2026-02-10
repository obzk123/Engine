add_executable(demo
    src/main.cpp
)

target_link_libraries(demo PRIVATE engine)

if (MSVC)
    target_compile_options(demo PRIVATE /W4 /permissive-)
else()
    target_compile_options(demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
