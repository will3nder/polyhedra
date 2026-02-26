cmake_minimum_required(VERSION 3.16)
project(polyhedra C)

set(CMAKE_C_STANDARD 99)

add_executable(polyhedra
    src/main.c
    src/shapes.c
    glad/src/glad.c
)

target_include_directories(polyhedra PRIVATE
    glad/include
)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

target_link_libraries(polyhedra
    glfw
    OpenGL::GL
)

if(UNIX)
    target_link_libraries(polyhedra m)
endif()
