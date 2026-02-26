#include "shapes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int load_shape(const char* filename, Polyhedron* shape) {
    FILE* file = fopen(filename, "r");
    if (!file) return 0;

    // Read Name and 4D Flag
    if (fscanf(file, "%31s %d", shape->name, &shape->is_4d) != 2) {
        fclose(file);
        return 0;
    }

    // Read Counts
    if (fscanf(file, "%d %d", &shape->v_count, &shape->e_count) != 2) {
        fclose(file);
        return 0;
    }

    // Allocate memory based on counts
    shape->vertices = (Vertex*)malloc(sizeof(Vertex) * shape->v_count);
    shape->edges = (Edge*)malloc(sizeof(Edge) * shape->e_count);

    // Parse data
    char line_type;
    int v_idx = 0, e_idx = 0;
    while (v_idx < shape->v_count || e_idx < shape->e_count) {
        if (fscanf(file, " %c", &line_type) != 1) break;

        if (line_type == 'v' && v_idx < shape->v_count) {
            fscanf(file, "%f %f %f %f", &shape->vertices[v_idx].x, &shape->vertices[v_idx].y, 
                                        &shape->vertices[v_idx].z, &shape->vertices[v_idx].w);
            v_idx++;
        } else if (line_type == 'e' && e_idx < shape->e_count) {
            fscanf(file, "%d %d", &shape->edges[e_idx].start, &shape->edges[e_idx].end);
            e_idx++;
        }
    }

    fclose(file);
    return 1;
}