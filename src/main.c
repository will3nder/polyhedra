#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shapes.h"

#define WIDTH  800
#define HEIGHT 600

// Global state (same as original code)
float angle_x = 0.0f, angle_y = 0.0f;
float angle_xw = 0.0f, angle_yw = 0.0f, angle_zw = 0.0f;
float fuzziness = 0.0f;
int current_shape_idx = 0;
int auto_rotate = 1;

// Structure to hold shape info (names for debugging/printing)
typedef struct {
    char name[32];
    int v_count;
    int e_count;
    Vertex *vertices;
    Edge *edges;
} Polyhedron;

int main() {
    // Initialize shape data (vertices & edges)
    init_icosahedron();
    init_dodecahedron();
    init_tesseract();
    init_truncated_octahedron();
    init_stella_octangula();
    init_sphere();
    init_mobius();
    init_hypersphere();

    // Set up shapes array
    Polyhedron shapes[9];
    // Shape 0: Icosahedron
    shapes[0].v_count = 12;
    shapes[0].e_count = 30;
    shapes[0].vertices = ico_v;
    shapes[0].edges = ico_e;
    strcpy(shapes[0].name, "Isocahedron");
    // Shape 1: Dodecahedron
    shapes[1].v_count = 20;
    shapes[1].e_count = 30;
    shapes[1].vertices = dod_v;
    shapes[1].edges = dod_e;
    strcpy(shapes[1].name, "Dodecahedron");
    // Shape 2: Cube
    shapes[2].v_count = 8;
    shapes[2].e_count = 12;
    shapes[2].vertices = cube_v;
    shapes[2].edges = cube_e;
    strcpy(shapes[2].name, "Cube");
    // Shape 3: Tesseract
    shapes[3].v_count = 16;
    shapes[3].e_count = 32;
    shapes[3].vertices = tes_v;
    shapes[3].edges = tes_e;
    strcpy(shapes[3].name, "Tesseract");
    // Shape 4: Truncated Octahedron
    shapes[4].v_count = 24;
    shapes[4].e_count = 36;
    shapes[4].vertices = toc_v;
    shapes[4].edges = toc_e;
    strcpy(shapes[4].name, "Truncated Octahedron");
    // Shape 5: Stella Octangula
    shapes[5].v_count = 12;
    shapes[5].e_count = 24;
    shapes[5].vertices = soc_v;
    shapes[5].edges = soc_e;
    strcpy(shapes[5].name, "Stella Octangula");
    // Shape 6: Sphere
    shapes[6].v_count = (SPHERE_RES_THETA+1)*(SPHERE_RES_PHI+1);
    shapes[6].e_count = SPHERE_RES_THETA*SPHERE_RES_PHI*2;
    shapes[6].vertices = sphere_v;
    shapes[6].edges = sphere_e;
    strcpy(shapes[6].name, "Sphere");
    // Shape 7: Mobius Strip
    shapes[7].v_count = MOBIUS_RES_U*MOBIUS_RES_V;
    shapes[7].e_count = MOBIUS_RES_U*MOBIUS_RES_V*2;
    shapes[7].vertices = mobius_v;
    shapes[7].edges = mobius_e;
    strcpy(shapes[7].name, "Mobius Strip");
    // Shape 8: Hypersphere
    shapes[8].v_count = HYPER_RES_THETA1*HYPER_RES_THETA2*HYPER_RES_PHI;
    shapes[8].e_count = actual_edges;
    shapes[8].vertices = hypersphere_v;
    shapes[8].edges = hypersphere_e;
    strcpy(shapes[8].name, "Hypersphere");

    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    // Request OpenGL 3.3 core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Polyhedra Wireframe", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to open GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Load OpenGL functions using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }
    glViewport(0, 0, WIDTH, HEIGHT);

    // Build simple shader program (vertex + fragment)
    const char *vertexShaderSource =
        "#version 330 core\n"
        "layout(location=0) in vec2 aPos;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "}\n";
    const char *fragmentShaderSource =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(1.0);\n"
        "}\n";

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // (Skipping error checks for brevity)

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Link shaders into program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // (Skipping error checks)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(shaderProgram);

    // Prepare VAOs/VBOs/EBOs for each shape
    GLuint VAOs[9], VBOs[9], EBOs[9];
    glGenVertexArrays(9, VAOs);
    glGenBuffers(9, VBOs);
    glGenBuffers(9, EBOs);
    for(int i = 0; i < 9; i++) {
        glBindVertexArray(VAOs[i]);
        // Vertex buffer (we will update it dynamically)
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[i]);
        glBufferData(GL_ARRAY_BUFFER, shapes[i].v_count * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Edge index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[i]);
        int edgeCount = shapes[i].e_count;
        int *indices = (int*)malloc(edgeCount * 2 * sizeof(int));
        for(int j = 0; j < edgeCount; j++) {
            indices[2*j]   = shapes[i].edges[j].start;
            indices[2*j+1] = shapes[i].edges[j].end;
        }
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, edgeCount * 2 * sizeof(int), indices, GL_STATIC_DRAW);
        free(indices);
        glBindVertexArray(0);
    }

    glLineWidth(2.0f);
    glClearColor(0.0, 0.0, 0.0, 1.0);

    // Allocate buffer for transformed vertices (max shape size = 1000)
    float vertexBuffer[2000];

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Input handling
        glfwPollEvents();
        // Shape selection (keys 1..9)
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) current_shape_idx = 0;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) current_shape_idx = 1;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) current_shape_idx = 2;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) current_shape_idx = 3;
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) current_shape_idx = 4;
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) current_shape_idx = 5;
        if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) current_shape_idx = 6;
        if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) current_shape_idx = 7;
        if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) current_shape_idx = 8;
        // Toggle auto-rotate
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            auto_rotate = !auto_rotate;
            // simple debounce
            while(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) glfwPollEvents();
        }
        // Quit on Q
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            break;
        }
        // Fuzziness adjustments
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            fuzziness += 0.05f;
            if(fuzziness > 10.0f) fuzziness = 9.95f;
        }
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            fuzziness -= 0.05f;
            if(fuzziness < 0.0f) fuzziness = 0.0f;
        }
        // Rotation keys
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) angle_x -= 0.1f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) angle_x += 0.1f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) angle_y += 0.1f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) angle_y -= 0.1f;
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) angle_xw -= 0.1f;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) angle_xw += 0.1f;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) angle_yw -= 0.1f;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) angle_yw += 0.1f;
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) angle_zw -= 0.1f;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) angle_zw += 0.1f;

        if (auto_rotate) {
            angle_x += 0.008f;
            angle_y += 0.012f;
            angle_xw += 0.006f;
            angle_yw += 0.004f;
            angle_zw += 0.01f;
        }

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Compute transformed vertices for current shape (with 4D projection if needed)
        Polyhedron *p = &shapes[current_shape_idx];
        for(int i = 0; i < p->v_count; i++) {
            // Original 4D coords
            float x = p->vertices[i].x;
            float y = p->vertices[i].y;
            float z = p->vertices[i].z;
            float w = p->vertices[i].w;

            // Apply fuzziness noise
            float noise = fuzziness * sinf((float)i * 132.0f + angle_x * 5.0f);
            x += noise; y += noise; z += noise; w += noise;

            // 4D Rotations (for tesseract and hypersphere)
            if (current_shape_idx == 3 || current_shape_idx == 8) {
                // Rotate in xw plane
                float cos_xw = cosf(angle_xw), sin_xw = sinf(angle_xw);
                float nx = x * cos_xw - w * sin_xw;
                float nw = x * sin_xw + w * cos_xw;
                x = nx; w = nw;
                // Rotate in yw plane
                float cos_yw = cosf(angle_yw), sin_yw = sinf(angle_yw);
                float ny = y * cos_yw - w * sin_yw;
                nw = y * sin_yw + w * cos_yw;
                y = ny; w = nw;
                // Rotate in zw plane
                float cos_zw = cosf(angle_zw), sin_zw = sinf(angle_zw);
                float nz = z * cos_zw - w * sin_zw;
                nw = z * sin_zw + w * cos_zw;
                z = nz; w = nw;
            }

            // Rotate around X axis
            float temp_y = y * cosf(angle_x) - z * sinf(angle_x);
            float temp_z = y * sinf(angle_x) + z * cosf(angle_x);
            y = temp_y; z = temp_z;

            // Rotate around Y axis
            float temp_x = x * cosf(angle_y) + z * sinf(angle_y);
            z = -x * sinf(angle_y) + z * cosf(angle_y);
            x = temp_x;

            // 4D -> 3D perspective (for tesseract/hypersphere)
            if (current_shape_idx == 3 || current_shape_idx == 8) {
                float w_factor = 1.0f / (2.0f - w * 0.3f);
                x *= w_factor;
                y *= w_factor;
                z *= w_factor;
            }

            // 3D perspective projection
            float distance = 4.0f;
            float factor = 50.0f / (distance - z * 0.5f);
            // Compute final (screen) coordinates; normalize for NDC
            float fx = x * factor * 2.0f / 40.0f;
            float fy = y * factor / 20.0f;
            // Store in vertex buffer (ignoring z)
            vertexBuffer[2*i] = fx;
            vertexBuffer[2*i+1] = fy;
        }

        // Update VBO for current shape
        glBindVertexArray(VAOs[current_shape_idx]);
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[current_shape_idx]);
        glBufferData(GL_ARRAY_BUFFER, p->v_count * 2 * sizeof(float), vertexBuffer, GL_DYNAMIC_DRAW);

        // Draw edges
        glDrawElements(GL_LINES, p->e_count * 2, GL_UNSIGNED_INT, 0);

        // Swap buffers
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

