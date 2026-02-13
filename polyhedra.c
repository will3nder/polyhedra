#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>

#define WIDTH 80
#define HEIGHT 40
#define PI 3.14159265389
#define PHI 1.61803398875

typedef struct {
    float x, y, z;
} Vertex;

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
float fuzziness = 0.0f;
int current_shape_idx = 0;
int auto_rotate = 1;

// CUBE

Vertex cube_v[] = {
    {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
    {-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1}
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
    ico_v[0] = (Vertex){0, 1, PHI}; ico_v[1] = (Vertex){0,-1, PHI};
    ico_v[2] = (Vertex){0, 1,-PHI}; ico_v[3] = (Vertex){0,-1,-PHI};
    // PlusMinus 1, PlusMinus PHI, 0
    ico_v[4] = (Vertex){1, PHI, 0}; ico_v[5] = (Vertex){-1, PHI, 0};
    ico_v[6] = (Vertex){1,-PHI, 0}; ico_v[7] = (Vertex){-1,-PHI, 0};
    // PlusMinus PHI, 0, PlusMinus 1
    ico_v[8] = (Vertex){PHI, 0, 1}; ico_v[9] = (Vertex){-PHI, 0, 1};
    ico_v[10] = (Vertex){PHI, 0, -1}; ico_v[11] = (Vertex){-PHI, 0, -1};

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
                dod_v[k++] = (Vertex){(float)x, (float)y, (float)z};
    
    // (0, PlusMinus 1/phi, PlusMinus phi)
    for(int i=-1; i<=1; i+=2)
        for(int j=-1; j<=1; j+=2)
            dod_v[k++] = (Vertex){0, (float)i/PHI, (float)j*PHI};

    // (PlusMinus 1/phi, PlusMinus phi, 0)
    for(int i=-1; i<=1; i+=2)
        for(int j=-1; j<=1; j+=2)
            dod_v[k++] = (Vertex){(float)i/PHI, (float)j*PHI, 0};
            
    // (PlusMinus phi, 0, PlusMinus 1/phi)
    for(int i=-1; i<=1; i+=2)
        for(int j=-1; j<=1; j+=2)
            dod_v[k++] = (Vertex){(float)i*PHI, 0, (float)j/PHI};

    // Calculate edges based on distance
    int e_idx = 0;
    for(int i=0; i<20; i++) {
        for(int j=i+1; j<20; j++) {
            float dist = sqrt(pow(dod_v[i].x - dod_v[j].x, 2) + 
                              pow(dod_v[i].y - dod_v[j].y, 2) + 
                              pow(dod_v[i].z - dod_v[j].z, 2));
            // Adjusted threshold for dodecahedron edge length
            if (fabs(dist - 1.0) < 0.01) { 
                 if(e_idx < 30) dod_e[e_idx++] = (Edge){i, j};
            }
        }
    }
}

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

void clear_screen() {
        printf("\033[H\033[J"); // ANSI clear 
}

void plot(char buffer[HEIGHT][WIDTH], int x, int y, char c) {
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                    buffer[y][x] = c;
                    }
}

// Breshnam's Line Algorithm
void draw_line(char buffer[HEIGHT][WIDTH], int x0, int y0, int x1, int y1) {
    int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while(1){
        plot(buffer, x0, y0, '#');
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if(e2 >= dy) { err += dy; x0 += sx; }
        if(e2 <= dx) { err += dx; y0 += sy; }
    }
}

// --- Main ---

int main() {
    init_icosahedron();
    init_dodecahedron();

    Polyhedron shapes[3];

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

    char buffer[HEIGHT][WIDTH];
    Vertex projected[100];

    printf("\033[?25l"); // Hide Cursor
    
    while (1) {
        // Input Handling
        if (kbhit()) { 
            char c = getchar();
            if (c == 'q') break;
            if (c == 'r') auto_rotate = !auto_rotate;
            if (c == '1') current_shape_idx = 0;
            if (c == '2') current_shape_idx = 1;
            if (c == '3') current_shape_idx = 2;
            if (c == 'f') fuzziness += 0.05;
            if (c == 'g') { fuzziness -= 0.05; if (fuzziness < 0) fuzziness = 0; }

            if (c == 'w') angle_x -= 0.1;
            if (c == 's') angle_x += 0.1;
            if (c == 'd') angle_y += 0.1;
            if (c == 'a') angle_y -= 0.1;
        }

        if (auto_rotate) {
            angle_x += 0.02;
            angle_y += 0.03;
        }

        memset(buffer, ' ', sizeof(buffer));

        Polyhedron *p = &shapes[current_shape_idx];
        
        // Transformation & Projection

        for (int i = 0; i < p->v_count; i++) {
            float x = p->vertices[i].x;
            float y = p->vertices[i].y;
            float z = p->vertices[i].z;

            // Apply Fuzziness(Noise)
            // Use sin/cos of index to make the noise change as we change hwo we look at it
            float noise = fuzziness * sin((float)i * 132.0f + angle_x * 5.0f);
            x += noise;
            y += noise;
            z += noise;

            // Rotate X
            float temp_y = y * cos(angle_x) - z * sin(angle_x);
            float temp_z = y * sin(angle_x) + z * cos(angle_x);
            y = temp_y;
            z = temp_z;

            // Rotate Y
            float temp_x = x * cos(angle_y) + z * sin(angle_y);
            z = -x * sin(angle_y) + z * cos(angle_y);
            x = temp_x;
            
            // Project (Weak Perspective)
            float distance = 4.0f;
            float factor = 20.0f / (distance - z * 0.5); // Zoom Factor
            
            int px = (int)(x * factor * 2.0 + WIDTH / 2);
            int py = (int)(y * factor + HEIGHT / 2);

            projected[i].x = px;
            projected[i].y = py;
        }

        // Draw Edges
        
        for (int i = 0; i < p->e_count; i++) {
            int s = p->edges[i].start;
            int e = p->edges[i].end;
            draw_line(buffer,
                  (int)projected[s].x, (int)projected[s].y,
                  (int)projected[e].x, (int)projected[e].y);
        }

        // Draw Vertices
        
        for (int i = 0; i < p->v_count; i++){
            plot(buffer, (int)projected[i].x, (int)projected[i].y, 'O');

        }

        // Render to Screen
        
        clear_screen();

        // UI Header
        printf("--- POLYHEDRA ---\n");
        printf("Shape: %s | V: %d | E: %d\n", p->name, p->v_count, p->e_count);
        printf("Fuzziness: %.2f (Press 'f' to increase, 'g' to decrease)\n", fuzziness);
        printf("Controls: [1-3] Change Shape | [WASD] Rotate | [r] Auto-Rotate | [q] Quit\n");
        printf("--------------------------------------------------\n");

        for (int y = 0; y < HEIGHT; y++){
            for (int x = 0; x < WIDTH; x++){
                putchar(buffer[y][x]);
            }
            putchar('\n');
        }

        usleep(30000);
    }

    printf("\033[?25h"); // Show cursor
    return 0;
}