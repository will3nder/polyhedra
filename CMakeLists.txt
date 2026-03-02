cmake_minimum_required(VERSION 3.16)
project(polyhedra C)

set(CMAKE_C_STANDARD 99)

add_executable(polyhedra
    src/main.c
    src/shapes.c
    libs/glad/src/glad.c
)

target_include_directories(polyhedra PRIVATE
    libs/glad/include
)

find_package(OpenGL REQUIRED)

target_link_libraries(polyhedra PRIVATE
    glfw
    OpenGL::GL
    $<$<PLATFORM_ID:Linux>:m>
)
