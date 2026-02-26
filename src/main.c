#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>
#include <string.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shapes.h"

#define WIDTH  1200
#define HEIGHT 800

// Global state 
float angle_x = 0.0f, angle_y = 0.0f;
float angle_xw = 0.0f, angle_yw = 0.0f, angle_zw = 0.0f;
float fuzziness = 0.0f;
int current_shape_idx = 0;
int auto_rotate = 1;


int main(int argc, char* argv[]) {
    // Check if shapes dir arg was sent
    if(argc < 2) {
        fprintf(stderr, "Usage: polyhedra [SHAPES_DIR]... [OPTION]...\n");
        return(-1);
    }

    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    // Request OpenGL 3.3 core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Polyhedra",  NULL, NULL);
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

    // Load shapes in from specified folder
    Polyhedron* shapes = NULL;
    int shape_count = 0;
    DIR *dir;
    if((dir = opendir(argv[1])) == NULL) {
        fprintf(stderr, "Failed to open directory: %s\n", argv[1]);
        return -1;
    }

    struct dirent *ent;
    char** files = NULL;
    int file_count = 0;

    // Collect filenames to sort
    while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, ".shape")) {
            files = realloc(files, sizeof(char*) * (file_count + 1));
            files[file_count] = strdup(ent->d_name);
            file_count++;
        }
    }
    closedir(dir);

    if (file_count == 0) {
        fprintf(stderr, "No shapes found in %s\n", argv[1]);
        return -1;
    }

    // Alphabetical Sort
    int cmp(const void *a, const void *b) {
        return strcmp(*(const char **)a, *(const char**)b);
    }
    qsort(files, file_count, sizeof(char*), cmp);

    // Load Individual Files
    for(int i = 0; i < file_count; i++) {
        char path[256];
        sprintf(path, "%s/%s", argv[1], files[i]);
        // Put found shapes into arr in alphabetical order
        shapes = realloc(shapes, sizeof(Polyhedron) * (shape_count + 1));
        if(load_shape(path, &shapes[shape_count])){
            shape_count++;
        } else {
            fprintf(stderr, "Shape \"%s/%s\" failed to intialize\n", argv[1], files[i]);
        }
    }


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
    GLuint *VAOs = malloc(sizeof(GLuint) * shape_count);
    GLuint *VBOs = malloc(sizeof(GLuint) * shape_count);
    GLuint *EBOs = malloc(sizeof(GLuint) * shape_count);
    glGenVertexArrays(shape_count, VAOs);
    glGenBuffers(shape_count, VBOs);
    glGenBuffers(shape_count, EBOs);
    for(int i = 0; i < shape_count; i++) {
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

    // Allocate buffer for transformed vertices
    float vertexBuffer[2500];

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Input handling
        glfwPollEvents();
        // Use Arrow Keys to Cycle
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            current_shape_idx = (current_shape_idx + 1) % shape_count;
            while(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) glfwPollEvents();
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            current_shape_idx = current_shape_idx == 0 ? shape_count-1 : current_shape_idx-1;
            while(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) glfwPollEvents();
        }

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
        // Rotation keys
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) angle_x -= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) angle_x += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) angle_y += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) angle_y -= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) angle_xw -= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) angle_xw += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) angle_yw -= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) angle_yw += 0.01f;
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) angle_zw -= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) angle_zw += 0.01f;

        if (auto_rotate) {
            angle_x += 0.004f;
            angle_y += 0.006f;
            angle_xw += 0.003f;
            angle_yw += 0.002f;
            angle_zw += 0.005f;
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

            // 4D Rotations (for tesseract and hypersphere)
            if (p->is_4d) {
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
            temp_z = -x * sinf(angle_y) + z * cosf(angle_y);
            x = temp_x; z = temp_z;

            // 4D -> 3D perspective (for tesseract/hypersphere)
            if (p->is_4d) {
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

