add_executable(demo
    src/main.cpp
)

target_link_libraries(demo PRIVATE engine)

if (MSVC)
    target_compile_options(demo PRIVATE /W4 /permissive-)
else()
    target_compile_options(demo PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Copiar assets al directorio del ejecutable despues de compilar ──
# Esto crea demo/assets/ al lado de demo.exe para que los paths relativos funcionen.
add_custom_command(TARGET demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:demo>/demo/assets"
    COMMENT "Copying demo assets..."
)
