#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "graph.h"

#define WIDTH 800
#define HEIGHT 600
#define NODE_RADIUS 25

void draw_arrow(Vector2 start, Vector2 end) {
    DrawLineEx(start, end, 3, BLACK);

    float angle = atan2f(end.y - start.y, end.x - start.x);

    float arrowLength = 18;
    float arrowWidth = 8;

    Vector2 left = {
        end.x - arrowLength * cosf(angle) + arrowWidth * sinf(angle),
        end.y - arrowLength * sinf(angle) - arrowWidth * cosf(angle)
    };

    Vector2 right = {
        end.x - arrowLength * cosf(angle) - arrowWidth * sinf(angle),
        end.y - arrowLength * sinf(angle) + arrowWidth * cosf(angle)
    };

    DrawTriangle(end, left, right, RED);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("ERROR: Wrong number of arguments\n");
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("error opening file");
        return 1;
    }

    int node_num;
    int edge_num;

    fscanf(file, "%d %d", &node_num, &edge_num);
    if (node_num > 15) {
       printf("ERROR: Maximum number of nodes is 15\n");
       fclose(file);
       return 1;
    }
    graph* g = create_graph(node_num);

    for (int i = 0; i < edge_num; i++) {
        int src, dest, weight;
        fscanf(file, "%d %d %d", &src, &dest, &weight);
        add_edge(g, src, dest, weight);
    }

    int source, destination;
    fscanf(file, "%d %d", &source, &destination);

    fclose(file);

    InitWindow(WIDTH, HEIGHT, "Graph GUI");

    Vector2 positions[15];

    float centerX = WIDTH / 2.0f;
    float centerY = HEIGHT / 2.0f;
    float layoutRadius = 200.0f;

    for (int i = 0; i < node_num; i++) {
        float angle = 2 * PI * i / node_num;
        positions[i].x = centerX + layoutRadius * cosf(angle);
        positions[i].y = centerY + layoutRadius * sinf(angle);
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int src = 0; src < node_num; src++) {
            edge* curr = g->adjacency_list[src];

            while (curr != NULL) {
                int dest = curr->dest;
                int weight = curr->weight;

                Vector2 start = positions[src];
                Vector2 end = positions[dest];

                float angle = atan2f(end.y - start.y, end.x - start.x);

                start.x += NODE_RADIUS * cosf(angle);
                start.y += NODE_RADIUS * sinf(angle);

                end.x -= NODE_RADIUS * cosf(angle);
                end.y -= NODE_RADIUS * sinf(angle);

                draw_arrow(start, end);

                int midX = (start.x + end.x) / 2;
                int midY = (start.y + end.y) / 2;

                char weightText[20];
                sprintf(weightText, "%d", weight);
                DrawText(weightText, midX, midY, 20, BLUE);

                curr = curr->next;
            }
        }

        for (int i = 0; i < node_num; i++) {
            DrawCircleV(positions[i], NODE_RADIUS, RED);

            char nodeText[10];
            sprintf(nodeText, "%d", i);

            DrawText(nodeText, positions[i].x - 6, positions[i].y - 10, 20, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    free_graph(g);

    return 0;
}