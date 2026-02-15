#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>

#define WIDTH 80
#define HEIGHT 40
#define PI 3.14159265389
#define PHI 1.61803398875

// Internal high resolution buffer (supersampled)
#define RENDER_SCALE 3
#define RWIDTH (WIDTH * RENDER_SCALE)
#define RHEIGHT (HEIGHT * RENDER_SCALE)

typedef struct {
    float x, y, z, w;
} Vertex;

typedef struct {
    float x, y;
    float depth;
} ProjectedVertex;

typedef struct {
    int start;
    int end;
} Edge;

typedef struct {
    char name[32];
    int v_count;
    int e_count;
    Vertex *vertices;
    Edge *edges;
} Polyhedron;

float angle_x = 0.0f;
float angle_y = 0.0f;
float angle_xw = 0.0f;
float angle_yw = 0.0f;
float angle_zw = 0.0f;
float fuzziness = 0.0f;
int current_shape_idx = 0;
int auto_rotate = 1;

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
    // 0, PlusMinus 1, PlusMinus PHI
    ico_v[0] = (Vertex){0, 1, PHI, 0}; ico_v[1] = (Vertex){0,-1, PHI, 0};
    ico_v[2] = (Vertex){0, 1,-PHI, 0}; ico_v[3] = (Vertex){0,-1,-PHI, 0};
    // PlusMinus 1, PlusMinus PHI, 0
    ico_v[4] = (Vertex){1, PHI, 0, 0}; ico_v[5] = (Vertex){-1, PHI, 0, 0};
    ico_v[6] = (Vertex){1,-PHI, 0, 0}; ico_v[7] = (Vertex){-1,-PHI, 0, 0};
    // PlusMinus PHI, 0, PlusMinus 1
    ico_v[8] = (Vertex){PHI, 0, 1, 0}; ico_v[9] = (Vertex){-PHI, 0, 1, 0};
    ico_v[10] = (Vertex){PHI, 0, -1, 0}; ico_v[11] = (Vertex){-PHI, 0, -1, 0};

    int k = 0;
    for(int i = 0; i < 12; i++) {
        for(int j = i+1; j < 12; j++) {
            float dist = pow(ico_v[i].x - ico_v[j].x, 2) +
                     pow(ico_v[i].y - ico_v[j].y, 2) +
                     pow(ico_v[i].z - ico_v[j].z, 2);
            if (dist < 4.1 && dist > 3.9) { //Edge length is 2
                if(k < 30) { ico_e[k++] = (Edge){i, j}; }
            }
        }
    }
}

// DODECAHEDRON - 12 Faces 20 Vertices

Vertex dod_v[20];
Edge dod_e[30];

void init_dodecahedron() {
	int k = 0;
	// (PlusMinus 1, PlusMinus 1, PlusMinus 1)
	for(int x=-1; x<=1; x+=2)
        for(int y=-1; y<=1; y+=2)
            for(int z=-1; z<=1; z+=2)
                dod_v[k++] = (Vertex){(float)x, (float)y, (float)z, 0};
    
    // (0, PlusMinus 1/phi, PlusMinus phi)
    for(int i=-1; i<=1; i+=2)
        for(int j=-1; j<=1; j+=2)
            dod_v[k++] = (Vertex){0, (float)i/PHI, (float)j*PHI, 0};

    // (PlusMinus 1/phi, PlusMinus phi, 0)
    for(int i=-1; i<=1; i+=2)
        for(int j=-1; j<=1; j+=2)
            dod_v[k++] = (Vertex){(float)i/PHI, (float)j*PHI, 0, 0};
            
    // (PlusMinus phi, 0, PlusMinus 1/phi)
    for(int i=-1; i<=1; i+=2)
        for(int j=-1; j<=1; j+=2)
            dod_v[k++] = (Vertex){(float)i*PHI, 0, (float)j/PHI, 0};

    // Calculate edges based on distance
    int e_idx = 0;
    for(int i=0; i<20; i++) {
        for(int j=i+1; j<20; j++) {
            float dist = sqrt(pow(dod_v[i].x - dod_v[j].x, 2) + 
                              pow(dod_v[i].y - dod_v[j].y, 2) + 
                              pow(dod_v[i].z - dod_v[j].z, 2));
            // Adjusted threshold for dodecahedron edge length (~1.236)
            if (fabs(dist - 1.236) < 0.1) { 
                 if(e_idx < 30) dod_e[e_idx++] = (Edge){i, j};
            }
        }
    }
}

// TRUNCATED OCTAHEDRON - 14 Faces 24 Vertices
Vertex toc_v[24];
Edge toc_e[36];

void init_truncated_octahedron() {
    int k = 0;
    float a = 1.0f / sqrt(2.0f);
    
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
    
    // Edges based on distance
    int e_idx = 0;
    for(int i=0; i<24; i++) {
        for(int j=i+1; j<24; j++) {
            float dist = sqrt(pow(toc_v[i].x - toc_v[j].x, 2) + 
                              pow(toc_v[i].y - toc_v[j].y, 2) + 
                              pow(toc_v[i].z - toc_v[j].z, 2));
            if (fabs(dist - (2.0f * a)) < 0.1f && e_idx < 36) {
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
    
    // Additional vertices for compound structure
    soc_v[8] = (Vertex){0.5f, 0.5f, 0.5f, 0};
    soc_v[9] = (Vertex){-0.5f, -0.5f, -0.5f, 0};
    soc_v[10] = (Vertex){0.5f, -0.5f, -0.5f, 0};
    soc_v[11] = (Vertex){-0.5f, 0.5f, 0.5f, 0};
    
    // Edges
    int e_idx = 0;
    for(int i=0; i<8; i++) {
        for(int j=i+1; j<8; j++) {
            float dist = sqrt(pow(soc_v[i].x - soc_v[j].x, 2) + 
                              pow(soc_v[i].y - soc_v[j].y, 2) + 
                              pow(soc_v[i].z - soc_v[j].z, 2));
            // Tetrahedron edge length is sqrt(8) ≈ 2.83
            if (fabs(dist - 2.828f) < 0.2f && e_idx < 24) {
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

// --- Sphere ---

#define SPHERE_RES_THETA 20
#define SPHERE_RES_PHI 20
Vertex sphere_v[(SPHERE_RES_THETA+1)*(SPHERE_RES_PHI+1)];
Edge sphere_e[(SPHERE_RES_THETA)*(SPHERE_RES_PHI)*2]; // Approx

void init_sphere(){
    int idx = 0;
    float r = 1.0f;
    for(int i = 0; i <= SPHERE_RES_THETA; i++){
        float theta = PI * i / SPHERE_RES_THETA;
        for(int j = 0; j <= SPHERE_RES_PHI; j++){
            float phi = 2*PI*j / SPHERE_RES_PHI;
            sphere_v[idx++] = (Vertex){
                r*sin(theta)*cos(phi),
                r*sin(theta)*sin(phi),
                r*cos(theta),
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
            int d = c+1;
            if(e_idx+4 < (SPHERE_RES_THETA)*(SPHERE_RES_PHI)*2){
                sphere_e[e_idx++] = (Edge){a,b};
                sphere_e[e_idx++] = (Edge){a,c};
            }
        }
    }
}

// Mobius Strip

#define MOBIUS_RES_U 50
#define MOBIUS_RES_V 10

Vertex mobius_v[MOBIUS_RES_U*MOBIUS_RES_V];
Edge mobius_e[MOBIUS_RES_U*MOBIUS_RES_V*2];

void init_mobius() {
    int idx = 0;
    float R = 1.0f, w = 0.3f;
    for(int i=0;i<MOBIUS_RES_U;i++){
        float t = 2*PI*i/MOBIUS_RES_U;
        for(int j=0;j<MOBIUS_RES_V;j++){
            float s = ((float)j/(MOBIUS_RES_V-1)-0.5f)*w;
            mobius_v[idx++] = (Vertex){
                (R + s*cos(t/2))*cos(t),
                (R + s*cos(t/2))*sin(t),
                s*sin(t/2),
                0
            };
        }
    }

    int e_idx = 0;
    for(int i=0;i<MOBIUS_RES_U;i++){
        for(int j=0;j<MOBIUS_RES_V;j++){
            int a = i*MOBIUS_RES_V + j;
            int b = i*MOBIUS_RES_V + (j+1)%MOBIUS_RES_V; // wrap V
            int c = ((i+1)%MOBIUS_RES_U)*MOBIUS_RES_V + j; // wrap U
            if(e_idx+2 < MOBIUS_RES_U*MOBIUS_RES_V*2){
                mobius_e[e_idx++] = (Edge){a,b};
                mobius_e[e_idx++] = (Edge){a,c};
            }
        }
    }
}

// --- Hypersphere (4D) ---
#define HYPER_RES_THETA1 10
#define HYPER_RES_THETA2 10
#define HYPER_RES_PHI 10

Vertex hypersphere_v[HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI];
Edge hypersphere_e[HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI*6];

void init_hypersphere(){
	int idx = 0;
	float r = 1.0f;
	for(int i = 0; i < HYPER_RES_THETA1; i++){
		float theta1 = PI * i / (HYPER_RES_THETA1-1);
		for(int j = 0; j < HYPER_RES_THETA2; j++){
			float theta2 = PI * i / (HYPER_RES_THETA2-1);
			for(int k = 0; k < HYPER_RES_PHI; k++){
				float phi = 2*PI*k / (HYPER_RES_PHI-1);
				hypersphere_v[idx++] = (Vertex){
					r*sin(theta1)*sin(theta2)*cos(phi),
					r*sin(theta1)*sin(theta2)*sin(phi),
					r*sin(theta1)*cos(theta2),
					r*cos(theta1)
				};
			}
		}
	}

	int e_idx = 0;
	int N = HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI;
	for(int i = 0; i < N; i++){
		for(int j = i+1; j < N; j++){
			int diff = 0;
			if(hypersphere_v[i].x != hypersphere_v[j].x) diff++;
			if(hypersphere_v[i].y != hypersphere_v[j].y) diff++;
			if(hypersphere_v[i].z != hypersphere_v[j].z) diff++;
			if(hypersphere_v[i].w != hypersphere_v[j].w) diff++;
			if(diff==1 && e_idx < N*6) hypersphere_e[e_idx++] = (Edge){i,j};
		}
	}
}

// 4D Rotation matrices (inline in main loop)
// These are now handled directly in the transformation code

// --- Terminal Utils ---

int kbhit(void){
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
        return 0;
}

// --- Drawing Utils ---

// Draw reference axes at bottom left corner
void draw_axes(char buffer[HEIGHT][WIDTH]) {
    int cx = 5, cy = HEIGHT - 5;  // corner position
    int len = 3;
    
    // X axis (horizontal) - using > chars
    for (int i = 0; i < len; i++) {
        if (cx + i < WIDTH && cy < HEIGHT) {
            buffer[cy][cx + i] = (i == len - 1) ? '>' : '~';
        }
    }
    
    // Y axis (vertical) - using ^ chars  
    for (int i = 0; i < len; i++) {
        if (cx < WIDTH && cy - i < HEIGHT) {
            buffer[cy - i][cx] = (i == len - 1) ? '^' : '~';
        }
    }
    
    // Z axis (diagonal) - using / chars
    for (int i = 0; i < len; i++) {
        if (cx - i < WIDTH && cy - i < HEIGHT && cx - i >= 0) {
            buffer[cy - i][cx - i] = (i == len - 1) ? '+' : '*';
        }
    }
    
    // Labels
    if (cx + len + 1 < WIDTH) buffer[cy][cx + len + 1] = 'X';
    if (cy - len - 1 >= 0) buffer[cy - len - 1][cx] = 'Y';
    if (cy - len - 1 >= 0 && cx - len - 1 >= 0) buffer[cy - len - 1][cx - len - 1] = 'Z';
}

char get_vertex_char(float depth) {
    // Use different characters based on depth
    // depth ranges from 0 (far) to 1+ (close)
    if (depth < 0.4f) return '.';
    if (depth < 0.6f) return 'o';
    if (depth < 0.8f) return 'O';
    return '@';
}

void clear_screen() {
        printf("\033[H\033[J"); // ANSI clear 
}

//void plot(char buffer[HEIGHT][WIDTH], int x, int y, char c) {
//        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
//                    buffer[y][x] = c;
//                    }
//}

float zbuffer[RHEIGHT][RWIDTH];

// void plot(char buffer[HEIGHT][WIDTH], float zbuffer[HEIGHT][WIDTH], int x, int y, char c, float depth) {
// 	if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
// 		if (depth > zbuffer[y][x]) {
// 			zbuffer[y][x] = depth;
// 			buffer[y][x] = c;
// 		}
// 	}
// }

void plot(float zbuffer[RHEIGHT][RWIDTH], 
          float depth_buffer[RHEIGHT][RWIDTH],
          int x, int y, float depth) {

    if (x >= 0 && x < RWIDTH && y >= 0 && y < RHEIGHT) {
        if (depth > depth_buffer[y][x]) {
            depth_buffer[y][x] = depth;
            zbuffer[y][x] = depth;
        }
    }
}

// Get character based on depth (for shading edges)
char get_line_char(float depth) {
    if (depth < 0.2f) return '.';
    if (depth < 0.4f) return '-';
    if (depth < 0.6f) return '=';
    if (depth < 0.8f) return '+';
    return '#';
}

// Depth-aware Bresenham's Line Algorithm
// Uses different characters based on depth for shading effect
//void draw_line_depth(char buffer[HEIGHT][WIDTH], int x0, int y0, float d0, int x1, int y1, float d1) {
//    int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
//   int dy = -abs(y1-y0), sy = y0 < y1 ? 1 : -1;
//    int err = dx + dy, e2;
//    float avg_depth = (d0 + d1) / 2.0f;
//    
//    // Get line character based on depth
//    char line_char = get_line_char(avg_depth);
//    int counter = 0;
//
//   while(1){
//        plot(buffer, x0, y0, line_char);
//        if (x0 == x1 && y0 == y1) break;
//        e2 = 2 * err;
//       if(e2 >= dy) { err += dy; x0 += sx; }
//        if(e2 <= dx) { err += dx; y0 += sy; }
//        counter++;
//    }
//}

// void draw_line_depth(char buffer[HEIGHT][WIDTH], float zbuffer[HEIGHT][WIDTH],
// 		     int x0, int y0, float d0,
// 		     int x1, int y1, float d1) {

// 	int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
// 	int dy = -abs(y1-y0), sy = y0 < y1 ? 1 : -1;
// 	int err = dx+dy, e2;

// 	int steps = dx > -dy ? dx : -dy;
// 	int step = 0;

// 	while(1){
// 		float t = steps ? (float)step / (float)steps : 0.0f;
// 		float depth = d0 + (d1-d0) * t;
// 		char line_char = get_line_char(depth);

// 		plot(buffer, zbuffer, x0, y0, line_char, depth);

// 		if (x0 == x1 && y0 == y1) break;
// 		e2 = 2 * err;
// 		if(e2 >= dy) { err += dy; x0 += sx; }
// 		if(e2 <= dx) { err += dx; y0 += sy; }
// 		step++;
// 	}
// }

void draw_line_depth(float render[RHEIGHT][RWIDTH],
                     float depth_buffer[RHEIGHT][RWIDTH],
                     int x0, int y0, float d0,
                     int x1, int y1, float d1) {

    int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    int steps = dx > -dy ? dx : -dy;
    int step = 0;

    while (1) {

        float t = steps ? (float)step / (float)steps : 0.0f;
        float depth = d0 + (d1 - d0) * t;

        plot(render, depth_buffer, x0, y0, depth);

        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }

        step++;
    }
}

// --- Main ---

int main() {
    init_icosahedron();
    init_dodecahedron();
    init_tesseract();
    init_truncated_octahedron();
    init_stella_octangula();
    init_sphere();
    init_mobius();
    init_hypersphere();

    Polyhedron shapes[9];

    // Shape 0 Isocahedron
    shapes[0].v_count = 12;
    shapes[0].e_count = 30;
    shapes[0].vertices = ico_v;
    shapes[0].edges = ico_e;
    strcpy(shapes[0].name, "Isocahedron");

    // Shape 1 Dodecahedron
    shapes[1].v_count = 20;
    shapes[1].e_count = 30;
    shapes[1].vertices = dod_v;
    shapes[1].edges = dod_e;
    strcpy(shapes[1].name, "Dodecahedron");
    
    // Shape 2 Cube
    shapes[2].v_count = 8;
    shapes[2].e_count = 12;
    shapes[2].vertices = cube_v;
    shapes[2].edges = cube_e;
    strcpy(shapes[2].name, "Cube");

    // Shape 3 Tesseract
    shapes[3].v_count = 16;
    shapes[3].e_count = 32;
    shapes[3].vertices = tes_v;
    shapes[3].edges = tes_e;
    strcpy(shapes[3].name, "Tesseract");

    // Shape 4 Truncated Octahedron
    shapes[4].v_count = 24;
    shapes[4].e_count = 36;
    shapes[4].vertices = toc_v;
    shapes[4].edges = toc_e;
    strcpy(shapes[4].name, "Truncated Octahedron");

    // Shape 5 Stella Octangula
    shapes[5].v_count = 12;
    shapes[5].e_count = 24;
    shapes[5].vertices = soc_v;
    shapes[5].edges = soc_e;
    strcpy(shapes[5].name, "Stella Octangula");

    // Shape 6 Sphere
    shapes[6].v_count = (SPHERE_RES_THETA+1)*(SPHERE_RES_PHI+1);
    shapes[6].e_count = SPHERE_RES_THETA*SPHERE_RES_PHI*2;
    shapes[6].vertices = sphere_v;
    shapes[6].edges = sphere_e;
    strcpy(shapes[6].name, "Sphere");

    // Shape 7 Mobius Strip
    shapes[7].v_count = MOBIUS_RES_U*MOBIUS_RES_V;
    shapes[7].e_count = MOBIUS_RES_U*MOBIUS_RES_V*2;
    shapes[7].vertices = mobius_v;
    shapes[7].edges = mobius_e;
    strcpy(shapes[7].name, "Mobius Strip");

    // Shape 8 Hypersphere
    shapes[8].v_count = HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI;
    shapes[8].e_count = HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI*6;
    shapes[8].vertices = hypersphere_v;
    shapes[8].edges = hypersphere_e;
    strcpy(shapes[8].name, "Hypersphere");

    char buffer[HEIGHT][WIDTH];
    float render[RHEIGHT][RWIDTH];
    ProjectedVertex projected[100];

    printf("\033[?25l"); // Hide Cursor
    
    while (1) {
        // Input Handling
        if (kbhit()) { 
            char c = getchar();
			c = tolower((unsigned char)c);
            if (c == 'q') break;
            if (c == 'r') auto_rotate = !auto_rotate;
            if (c == '1') current_shape_idx = 0;
            if (c == '2') current_shape_idx = 1;
            if (c == '3') current_shape_idx = 2;
            if (c == '4') current_shape_idx = 3;
            if (c == '5') current_shape_idx = 4;
            if (c == '6') current_shape_idx = 5;
            if (c == '7') current_shape_idx = 6;
            if (c == '8') current_shape_idx = 7;
	    if (c == '9') current_shape_idx = 8;
            if (c == 'f') { fuzziness += 0.05; if (fuzziness >= 10) fuzziness = 9.95f; }
            if (c == 'g') { fuzziness -= 0.05; if (fuzziness < 0) fuzziness = 0; }

            if (c == 'w') angle_x -= 0.1;
            if (c == 's') angle_x += 0.1;
            if (c == 'd') angle_y += 0.1;
            if (c == 'a') angle_y -= 0.1;
            if (c == 'i') angle_xw -= 0.1;
            if (c == 'k') angle_xw += 0.1;
            if (c == 'j') angle_yw -= 0.1;
            if (c == 'l') angle_yw += 0.1;
            if (c == 'u') angle_zw -= 0.1;
            if (c == 'o') angle_zw += 0.1;
        }

        if (auto_rotate) {
            angle_x += 0.008;
            angle_y += 0.012;
            angle_xw += 0.006;
            angle_yw += 0.004;
            angle_zw += 0.01;
        }

        memset(buffer, ' ', sizeof(buffer));

        // for (int y = 0; y < HEIGHT; y++) {
        //     for (int x = 0; x < WIDTH; x++) {
        //         zbuffer[y][x] = -1e9f;
        //     }
        // }

        for (int y = 0; y < RHEIGHT; y++) {
            for (int x = 0; x < RWIDTH; x++) {
                render[y][x] = 0.0f;
                zbuffer[y][x] = -1e9f;
            }
        }


        Polyhedron *p = &shapes[current_shape_idx];
        
        // Transformation & Projection

        for (int i = 0; i < p->v_count; i++) {
            float x = p->vertices[i].x;
            float y = p->vertices[i].y;
            float z = p->vertices[i].z;
            float w = p->vertices[i].w;

            // Apply Fuzziness(Noise)
            // Use sin/cos of index to make the noise change as we change hwo we look at it
            float noise = fuzziness * sin((float)i * 132.0f + angle_x * 5.0f);
            x += noise;
            y += noise;
            z += noise;
            w += noise;

            // 4D Rotations (only for 4D objects)
            if (current_shape_idx == 3 || current_shape_idx == 8) {
                // Rotate in xw plane
                float cos_xw = cos(angle_xw), sin_xw = sin(angle_xw);
                float new_x = x * cos_xw - w * sin_xw;
                float new_w = x * sin_xw + w * cos_xw;
                x = new_x;
                w = new_w;
                
                // Rotate in yw plane
                float cos_yw = cos(angle_yw), sin_yw = sin(angle_yw);
                float new_y = y * cos_yw - w * sin_yw;
                new_w = y * sin_yw + w * cos_yw;
                y = new_y;
                w = new_w;
                
                // Rotate in zw plane
                float cos_zw = cos(angle_zw), sin_zw = sin(angle_zw);
                float new_z = z * cos_zw - w * sin_zw;
                new_w = z * sin_zw + w * cos_zw;
                z = new_z;
                w = new_w;
            }

            // Rotate X
            float temp_y = y * cos(angle_x) - z * sin(angle_x);
            float temp_z = y * sin(angle_x) + z * cos(angle_x);
            y = temp_y;
            z = temp_z;

            // Rotate Y
            float temp_x = x * cos(angle_y) + z * sin(angle_y);
            z = -x * sin(angle_y) + z * cos(angle_y);
            x = temp_x;
            
            // Project 4D->3D then 3D->2D
            // First project w coordinate to z
            float distance = 4.0f;
            float w_factor = 1.0f / (2.0f - w * 0.3f); // 4D perspective
            x *= w_factor;
            y *= w_factor;
            z *= w_factor;
            
            float factor = 50.0f / (distance - z * 0.5); // Zoom Factor
            
            // Calculate depth for later use (normalized 0-1, where 1 is closest)
            float depth = 1.0f / (1.0f + fabsf(z) * 0.5f);
            
            // int px = (int)(x * factor * 2.0 + WIDTH / 2);
            // int py = (int)(y * factor + HEIGHT / 2);

            int px = (int)(x * factor * 2.0 * RENDER_SCALE + RWIDTH / 2);
            int py = (int)(y * factor * RENDER_SCALE + RHEIGHT / 2);

            projected[i].x = px;
            projected[i].y = py;
            projected[i].depth = depth;
        }
        // Draw Edges with depth-based sampling
        for (int i = 0; i < p->e_count; i++) {
            int s = p->edges[i].start;
            int e = p->edges[i].end;
            draw_line_depth(render, zbuffer,
                  (int)projected[s].x, (int)projected[s].y, projected[s].depth,
                  (int)projected[e].x, (int)projected[e].y, projected[e].depth);
        }
        // Draw Vertices with depth-based characters
        // Not used for new rendering style (supersampling)
        
        // for (int i = 0; i < p->v_count; i++){
        //     char vc = get_vertex_char(projected[i].depth);
        //     plot(buffer, zbuffer, (int)projected[i].x, (int)projected[i].y, vc, projected[i].depth);

        // }

        const char *ramp = " .:-=+*#%@";

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {

                float max_depth = -1e9f;
                for (int sy = 0; sy < RENDER_SCALE; sy++){
                    for (int sx = 0; sx < RENDER_SCALE; sx++){
                        int rx = x * RENDER_SCALE + sx;
                        int ry = y * RENDER_SCALE + sy;
                        if (zbuffer[ry][rx] > max_depth){
                            max_depth = zbuffer[ry][rx];
                        }
                    }
                }

                if(max_depth > -1e8f) {
                    int idx = (int)(max_depth * 9.0f);
                    if (idx < 0) idx = 0;
                    if (idx > 9) idx = 9;
                    buffer[y][x] = ramp[idx];
                } else {
                    buffer[y][x] = ' ';
                }
            }
        }

        // Render to Screen - build entire output as single string with fixed positioning
        
        char output[15000];
        int offset = 0;
        offset += sprintf(output + offset, "\033[H\033[J"); // Clear screen
        
        // Position UI at top (stays fixed)
        float rot_x = fmod(angle_x * 57.2958f, 360.0f);
        float rot_y = fmod(angle_y * 57.2958f, 360.0f);
        if (rot_x < 0) rot_x += 360.0f;
        if (rot_y < 0) rot_y += 360.0f;
        
        offset += sprintf(output + offset, "\033[1;1H"); // Move to 1,1
        offset += sprintf(output + offset, "╔═══════ POLYHEDRA ════════╗");
        offset += sprintf(output + offset, "\033[2;1H");
        offset += sprintf(output + offset, "║ Shape: %-17s ║", p->name);
        offset += sprintf(output + offset, "\033[3;1H");
        offset += sprintf(output + offset, "║ V:%-2d E:%-2d R:%.0f°,%.0f° ║", p->v_count, p->e_count, rot_x, rot_y);
        offset += sprintf(output + offset, "\033[4;1H");
        offset += sprintf(output + offset, "║ Distort: %.2f            ║", fuzziness);
        offset += sprintf(output + offset, "\033[5;1H");
        offset += sprintf(output + offset, "╚══════════════════════════╝");
        
        // Geometry and buffer output in main area (starting at line 7)
        offset += sprintf(output + offset, "\033[7;1H");

        for (int y = 0; y < HEIGHT; y++){
            // Position each line
            if (y > 0) offset += sprintf(output + offset, "\033[%d;1H", 7 + y);
            for (int x = 0; x < WIDTH; x++){
                output[offset++] = buffer[y][x];
            }
        }
        
        // Controls at bottom
        offset += sprintf(output + offset, "\033[38;1H");
        offset += sprintf(output + offset, "┌─ SHAPE SELECT ─┐");
        offset += sprintf(output + offset, "\033[39;1H");
        offset += sprintf(output + offset, "│ [1-6] Select  [WASD] Rotate");
        offset += sprintf(output + offset, "\033[40;1H");
        offset += sprintf(output + offset, "│ [R] Auto-Rotate  [F/G] Distort");
        offset += sprintf(output + offset, "\033[41;1H");
        offset += sprintf(output + offset, "└────────────── [Q] Quit ────┘");
        output[offset] = '\0';
        
        fputs(output, stdout);
        fflush(stdout);
        usleep(15000);
    }
    
    printf("\033c"); // Clear Terminal
    printf("\033[?25h"); // Show cursor
    return 0;
}
