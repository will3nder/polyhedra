cmake_minimum_required(VERSION 3.16)
project(polyhedra)

set(CMAKE_C_STANDARD 11)

add_executable(polyhedra
	src/main.c
	src/shapes.c
	glad/src/glad.c
)

target_include_directories(polyhedra PRIVATE
	glad/include
)

find_package(OpenGL REQUIRED)

target_link_libraries(polyhedra
	OpenGL::GL
	glfw
	m
	${CMAKE_DL_LIBS}
)
