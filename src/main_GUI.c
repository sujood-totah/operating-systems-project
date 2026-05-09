#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "graph.h"
#include "graph_io.h"

#define WIDTH 800
#define HEIGHT 600
#define NODE_RADIUS 25
#define GUI_MAX_NODES 15

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
        printf("Invalid input\n");
        return 1;
    }

    graph_load_data data;
    graph_load_result r = graph_load_from_path(argv[1], &data, GUI_MAX_NODES);

    if (r == GRAPH_LOAD_NEGATIVE_WEIGHT) {
        printf("Negative weights are not allowed.\n");
        return 1;
    }
    if (r != GRAPH_LOAD_OK) {
        printf("Invalid input\n");
        return 1;
    }

    graph* g = data.g;
    const int node_num = g->node_num;

    (void)data.source;
    (void)data.destination;

    InitWindow(WIDTH, HEIGHT, "Graph GUI");
    SetTargetFPS(60);

    Vector2 positions[GUI_MAX_NODES];

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

                int midX = (int)((start.x + end.x) / 2);
                int midY = (int)((start.y + end.y) / 2);

                char weightText[32];
                const int fontSize = 20;
                snprintf(weightText, sizeof(weightText), "%d", weight);
                int tw = MeasureText(weightText, fontSize);
                DrawText(weightText, midX - tw / 2, midY - fontSize / 2, fontSize, BLUE);

                curr = curr->next;
            }
        }

        for (int i = 0; i < node_num; i++) {
            DrawCircleV(positions[i], NODE_RADIUS, RED);

            char nodeText[16];
            const int fontSize = 20;
            snprintf(nodeText, sizeof(nodeText), "%d", i);
            int tw = MeasureText(nodeText, fontSize);
            DrawText(nodeText,
                     (int)(positions[i].x - tw / 2),
                     (int)(positions[i].y - fontSize / 2),
                     fontSize,
                     WHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    free_graph(g);

    return 0;
}