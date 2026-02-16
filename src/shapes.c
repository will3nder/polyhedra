#include "shapes.h"
#include <stdlib.h>
#include <string.h>

// CUBE
Vertex cube_v[] = {
    {-1,-1,-1,0}, {1,-1,-1,0}, {1,1,-1,0}, {-1,1,-1,0},
    {-1,-1,1,0}, {1,-1,1,0}, {1,1,1,0}, {-1,1,1,0}
};
Edge cube_e[] = {
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
};

// ICOSAHEDRON - 20 Faces 12 Vertices - Golden Ratio Construction
Vertex ico_v[12];
Edge ico_e[30];
void init_icosahedron() {
    // 0, ±1, ±PHI
    ico_v[0] = (Vertex){0, 1, PHI, 0};  ico_v[1] = (Vertex){0,-1, PHI, 0};
    ico_v[2] = (Vertex){0, 1,-PHI, 0};  ico_v[3] = (Vertex){0,-1,-PHI, 0};
    // ±1, ±PHI, 0
    ico_v[4] = (Vertex){1, PHI, 0, 0};  ico_v[5] = (Vertex){-1, PHI, 0, 0};
    ico_v[6] = (Vertex){1,-PHI, 0, 0};  ico_v[7] = (Vertex){-1,-PHI, 0, 0};
    // ±PHI, 0, ±1
    ico_v[8] = (Vertex){PHI, 0, 1, 0};  ico_v[9] = (Vertex){-PHI, 0, 1, 0};
    ico_v[10] = (Vertex){PHI, 0, -1, 0}; ico_v[11] = (Vertex){-PHI, 0, -1, 0};

    int k = 0;
    for(int i = 0; i < 12; i++){
        for(int j = i+1; j < 12; j++){
            float dx = ico_v[i].x - ico_v[j].x;
            float dy = ico_v[i].y - ico_v[j].y;
            float dz = ico_v[i].z - ico_v[j].z;
            float dist = dx*dx + dy*dy + dz*dz;
            // edge length ~ 4
            if (dist < 4.1f && dist > 3.9f) {
                if (k < 30) ico_e[k++] = (Edge){i, j};
            }
        }
    }
}

// DODECAHEDRON - 12 Faces 20 Vertices
Vertex dod_v[20];
Edge dod_e[30];
void init_dodecahedron() {
    int k = 0;
    // (±1, ±1, ±1)
    for(int x=-1; x<=1; x+=2)
      for(int y=-1; y<=1; y+=2)
        for(int z=-1; z<=1; z+=2)
          dod_v[k++] = (Vertex){(float)x, (float)y, (float)z, 0};

    // (0, ±1/phi, ±phi)
    for(int i=-1; i<=1; i+=2)
      for(int j=-1; j<=1; j+=2)
        dod_v[k++] = (Vertex){0, (float)i/PHI, (float)j*PHI, 0};

    // (±1/phi, ±phi, 0)
    for(int i=-1; i<=1; i+=2)
      for(int j=-1; j<=1; j+=2)
        dod_v[k++] = (Vertex){(float)i/PHI, (float)j*PHI, 0, 0};

    // (±phi, 0, ±1/phi)
    for(int i=-1; i<=1; i+=2)
      for(int j=-1; j<=1; j+=2)
        dod_v[k++] = (Vertex){(float)i*PHI, 0, (float)j/PHI, 0};

    // Edges: threshold edge length ~1.236
    int e_idx = 0;
    for(int i = 0; i < 20; i++) {
        for(int j = i+1; j < 20; j++) {
            float dx = dod_v[i].x - dod_v[j].x;
            float dy = dod_v[i].y - dod_v[j].y;
            float dz = dod_v[i].z - dod_v[j].z;
            float dist = sqrtf(dx*dx + dy*dy + dz*dz);
            if (fabsf(dist - 1.236f) < 0.1f && e_idx < 30) {
                dod_e[e_idx++] = (Edge){i, j};
            }
        }
    }
}

// TRUNCATED OCTAHEDRON - 14 Faces 24 Vertices
Vertex toc_v[24];
Edge toc_e[36];
void init_truncated_octahedron() {
    int k = 0;
    float a = 1.0f / sqrtf(2.0f);

    // 24 vertices of truncated octahedron
    // (±1, ±a, ±a), permutations
    for(int i=-1; i<=1; i+=2) {
        toc_v[k++] = (Vertex){(float)i, a, a, 0};
        toc_v[k++] = (Vertex){(float)i, a, -a, 0};
        toc_v[k++] = (Vertex){(float)i, -a, a, 0};
        toc_v[k++] = (Vertex){(float)i, -a, -a, 0};
    }
    for(int i=-1; i<=1; i+=2) {
        toc_v[k++] = (Vertex){a, (float)i, a, 0};
        toc_v[k++] = (Vertex){a, (float)i, -a, 0};
        toc_v[k++] = (Vertex){-a, (float)i, a, 0};
        toc_v[k++] = (Vertex){-a, (float)i, -a, 0};
    }
    for(int i=-1; i<=1; i+=2) {
        toc_v[k++] = (Vertex){a, a, (float)i, 0};
        toc_v[k++] = (Vertex){a, -a, (float)i, 0};
        toc_v[k++] = (Vertex){-a, a, (float)i, 0};
        toc_v[k++] = (Vertex){-a, -a, (float)i, 0};
    }

    // Edges: length ~ 2*a
    int e_idx = 0;
    for(int i = 0; i < 24; i++) {
        for(int j = i+1; j < 24; j++) {
            float dx = toc_v[i].x - toc_v[j].x;
            float dy = toc_v[i].y - toc_v[j].y;
            float dz = toc_v[i].z - toc_v[j].z;
            float dist = sqrtf(dx*dx + dy*dy + dz*dz);
            if (fabsf(dist - (2.0f * a)) < 0.1f && e_idx < 36) {
                toc_e[e_idx++] = (Edge){i, j};
            }
        }
    }
}

// STELLA OCTANGULA (Compound of two tetrahedra) - 8 Faces 12 Vertices (unique)
Vertex soc_v[12];
Edge soc_e[24];
void init_stella_octangula() {
    // Two interpenetrating tetrahedra
    soc_v[0] = (Vertex){1, 1, 1, 0};
    soc_v[1] = (Vertex){1, -1, -1, 0};
    soc_v[2] = (Vertex){-1, 1, -1, 0};
    soc_v[3] = (Vertex){-1, -1, 1, 0};

    soc_v[4] = (Vertex){-1, -1, -1, 0};
    soc_v[5] = (Vertex){-1, 1, 1, 0};
    soc_v[6] = (Vertex){1, -1, 1, 0};
    soc_v[7] = (Vertex){1, 1, -1, 0};

    // Additional vertices for compound structure (not used in edges below)
    soc_v[8]  = (Vertex){0.5f, 0.5f, 0.5f, 0};
    soc_v[9]  = (Vertex){-0.5f, -0.5f, -0.5f, 0};
    soc_v[10] = (Vertex){0.5f, -0.5f, -0.5f, 0};
    soc_v[11] = (Vertex){-0.5f, 0.5f, 0.5f, 0};

    // Edges: tetrahedron edge length sqrt(8) ~ 2.828
    int e_idx = 0;
    for(int i = 0; i < 8; i++) {
        for(int j = i+1; j < 8; j++) {
            float dx = soc_v[i].x - soc_v[j].x;
            float dy = soc_v[i].y - soc_v[j].y;
            float dz = soc_v[i].z - soc_v[j].z;
            float dist = sqrtf(dx*dx + dy*dy + dz*dz);
            if (fabsf(dist - 2.828f) < 0.2f && e_idx < 24) {
                soc_e[e_idx++] = (Edge){i, j};
            }
        }
    }
}

// TESSERACT - 4D Hypercube - 16 Vertices 32 Edges
Vertex tes_v[16];
Edge tes_e[32];
void init_tesseract() {
    int k = 0;
    // All combinations of (±1, ±1, ±1, ±1)
    for(int x=-1; x<=1; x+=2)
        for(int y=-1; y<=1; y+=2)
            for(int z=-1; z<=1; z+=2)
                for(int w=-1; w<=1; w+=2)
                    tes_v[k++] = (Vertex){(float)x, (float)y, (float)z, (float)w};

    // Edges connect vertices that differ in exactly one coordinate
    int e_idx = 0;
    for(int i = 0; i < 16; i++) {
        for(int j = i+1; j < 16; j++) {
            int diff_count = 0;
            if(tes_v[i].x != tes_v[j].x) diff_count++;
            if(tes_v[i].y != tes_v[j].y) diff_count++;
            if(tes_v[i].z != tes_v[j].z) diff_count++;
            if(tes_v[i].w != tes_v[j].w) diff_count++;
            if(diff_count == 1 && e_idx < 32) {
                tes_e[e_idx++] = (Edge){i, j};
            }
        }
    }
}

// SPHERE
Vertex sphere_v[(SPHERE_RES_THETA+1)*(SPHERE_RES_PHI+1)];
Edge sphere_e[SPHERE_RES_THETA*SPHERE_RES_PHI*2];
void init_sphere(){
    int idx = 0;
    float r = 1.0f;
    for(int i = 0; i <= SPHERE_RES_THETA; i++){
        float theta = PI * i / SPHERE_RES_THETA;
        for(int j = 0; j <= SPHERE_RES_PHI; j++){
            float phi = 2*PI*j / SPHERE_RES_PHI;
            sphere_v[idx++] = (Vertex){
                r*sinf(theta)*cosf(phi),
                r*sinf(theta)*sinf(phi),
                r*cosf(theta),
                0
            };
        }
    }
    int e_idx = 0;
    for(int i = 0; i < SPHERE_RES_THETA; i++) {
        for(int j = 0; j < SPHERE_RES_PHI; j++) {
            int a = i*(SPHERE_RES_PHI+1)+j;
            int b = a+1;
            int c = a + (SPHERE_RES_PHI+1);
            // Only connect in one direction for wireframe
            sphere_e[e_idx++] = (Edge){a, b};
            sphere_e[e_idx++] = (Edge){a, c};
        }
    }
}

// MOBIUS STRIP
Vertex mobius_v[MOBIUS_RES_U*MOBIUS_RES_V];
Edge mobius_e[MOBIUS_RES_U*MOBIUS_RES_V*2];
void init_mobius() {
    int idx = 0;
    float R = 1.0f, w = 0.3f;
    for(int i=0; i<MOBIUS_RES_U; i++){
        float t = 2*PI*i / MOBIUS_RES_U;
        for(int j=0; j<MOBIUS_RES_V; j++){
            float s = ((float)j/(MOBIUS_RES_V-1)-0.5f)*w;
            mobius_v[idx++] = (Vertex){
                (R + s*cosf(t/2))*cosf(t),
                (R + s*cosf(t/2))*sinf(t),
                s*sinf(t/2),
                0
            };
        }
    }
    int e_idx = 0;
    for(int i=0; i<MOBIUS_RES_U; i++){
        for(int j=0; j<MOBIUS_RES_V; j++){
            int a = i*MOBIUS_RES_V + j;
            int b = i*MOBIUS_RES_V + (j+1)%MOBIUS_RES_V;
            int c = ((i+1)%MOBIUS_RES_U)*MOBIUS_RES_V + j;
            mobius_e[e_idx++] = (Edge){a, b};
            mobius_e[e_idx++] = (Edge){a, c};
        }
    }
}

// HYPERSPHERE (4D)
Vertex hypersphere_v[HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI];
Edge hypersphere_e[HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI*6];
int actual_edges;
void init_hypersphere(){
    float target_edge_length = 2.0f * sinf(PI / HYPER_RES_PHI);
    int idx = 0;
    float r = 1.0f;
    for(int i = 0; i < HYPER_RES_THETA1; i++){
        float theta1 = PI * i / (HYPER_RES_THETA1-1);
        for(int j = 0; j < HYPER_RES_THETA2; j++){
            float theta2 = PI * j / (HYPER_RES_THETA2-1);
            for(int k = 0; k < HYPER_RES_PHI; k++){
                float phi = 2*PI*k / (HYPER_RES_PHI-1);
                hypersphere_v[idx++] = (Vertex){
                    r*sinf(theta1)*sinf(theta2)*cosf(phi),
                    r*sinf(theta1)*sinf(theta2)*sinf(phi),
                    r*sinf(theta1)*cosf(theta2),
                    r*cosf(theta1)
                };
            }
        }
    }
    int e_idx = 0;
    int N = HYPER_RES_THETA1 * HYPER_RES_THETA2 * HYPER_RES_PHI;
    for(int i = 0; i < N; i++){
        for(int j = i+1; j < N; j++){
            float dx = hypersphere_v[i].x - hypersphere_v[j].x;
            float dy = hypersphere_v[i].y - hypersphere_v[j].y;
            float dz = hypersphere_v[i].z - hypersphere_v[j].z;
            float dw = hypersphere_v[i].w - hypersphere_v[j].w;
            float dist = sqrtf(dx*dx + dy*dy + dz*dz + dw*dw);
            if(fabsf(dist - target_edge_length) < 0.2f && e_idx < HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI*6) { 
                hypersphere_e[e_idx++] = (Edge){i, j};
            }
        }
    }
    actual_edges = e_idx;
}
