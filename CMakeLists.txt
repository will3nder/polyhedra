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

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(libs/glfw)

target_link_libraries(polyhedra PRIVATE
    glfw
    OpenGL::GL
    $<$<PLATFORM_ID:Linux>:m>
)
