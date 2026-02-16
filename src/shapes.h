#ifndef SHAPES_H
#define SHAPES_h

#include <math.h>

#define PI 3.14159265359f
#define PHI 1.61803398875f

// Sphere surface resolution
#define SPHERE_RES_THETA 20
#define SPHERE_RES_PHI 20

// Mobius Strip resolution
#define MOBIUS_RES_U 50
#define MOBIUS_RES_V 10

// Hypersphere (4D) resolution
#define HYPER_RES_THETA1 10
#define HYPER_RES_THETA2 10
#define HYPER_RES_PHI 10

typedef struct {
	float x, y, z, w;
} Vertex;

typedef struct {
	int start, end;
} Edge;

// CUBE
extern Vertex cube_v[8];
extern Edge cube_e[12];

// ICOSAHEDRON - 20 Faces 12 Vertices - Golden Ration Construction
extern Vertex ico_v[12];
extern Edge ico_e[30];
void init_icosahedron();

// DODECAHEDRON - 12 Faces 20 Vertices
extern Vertex dod_v[20];
extern Edge dod_e[30];
void init_dodecahedron();

// TRUNCATED OCTAHEDRON - 14 Faces 24 Vertices
extern Vertex toc_v[24];
extern Edge toc_e[36];
void init_truncated_octahedron();

// STELLA OCTANGULA (Compound of two tetrahedra) - 8 Faces 12 Vertices (unique)
extern Vertex soc_v[12];
extern Edge soc_e[24];
void init_stella_octangula();

// TESSERACT - 4D Hypercube - 16 Vertices 32 Edges
extern Vertex tes_v[16];
extern Edge tes_e[32];
void init_tesseract();

// SPHERE
extern Vertex sphere_v[(SPHERE_RES_THETA+1)*(SPHERE_RES_PHI+1)];
extern Edge sphere_e[SPHERE_RES_THETA*SPHERE_RES_PHI*2];
void init_sphere();

// MOBIUS STRIP
extern Vertex mobius_v[MOBIUS_RES_U*MOBIUS_RES_V];
extern Edge mobius_e[MOBIUS_RES_U*MOBIUS_RES_V*2];
void init_mobius();

// HYPERSPHERE (4D) - We also track actual_edges
extern Vertex hypersphere_v[HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI];
extern Edge hypersphere_e[HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI*6];
extern int actual_edges;
void init_hypersphere();

#endif
