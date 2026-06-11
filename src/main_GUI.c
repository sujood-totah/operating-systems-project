#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "graph.h"
#include "graph_io.h"
#include "dijkstra.h"

#define WIDTH 800
#define HEIGHT 600
#define NODE_RADIUS 25
#define GUI_MAX_NODES 15
#define EDGE_STEP_SEC 0.3f
#define NODE_WAIT_SEC 1.0f

static int edge_weight_between(graph* g, int u, int v) {
    for (edge* e = g->adjacency_list[u]; e != NULL; e = e->next) {
        if (e->dest == v) {
            return e->weight;
        }
    }
    return 1;
}

static int edge_on_shortest_path(int u, int v, const int* path, int path_len) {
    for (int i = 0; i < path_len - 1; i++) {
        if (path[i] == u && path[i + 1] == v) {
            return 1;
        }
    }
    return 0;
}

static void draw_arrow_colored(Vector2 start, Vector2 end, float thick, Color lineCol,
                               Color headCol) {
    DrawLineEx(start, end, thick, lineCol);

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

    DrawTriangle(end, left, right, headCol);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Invalid input\n");
        return 1;
    }

    graph_load_data data;
    graph_load_result r = graph_load_from_path(argv[1], &data, GUI_MAX_NODES);

    if (r == GRAPH_LOAD_NEGATIVE_WEIGHT) {
        printf("Invalid input\n");
        return 1;
    }
    if (r != GRAPH_LOAD_OK) {
        printf("Invalid input\n");
        return 1;
    }

    graph* g = data.g;
    const int node_num = g->node_num;
    const int src = data.source;
    const int dest = data.destination;

    int path[GUI_MAX_NODES];
    int path_length = 0;
    int total_distance = 0;

    int has_path = dijkstra_get_path(g, src, dest, path, &path_length, &total_distance);

    if (!has_path) {
        printf("No path found\n");
        free_graph(g);
        return 1;
    }

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

    int is_playing = 0;
    int current_edge_index = 0;
    int current_step = 0;
    float step_timer = 0.0f;
    float wait_timer = 0.0f;
    int is_waiting = 0;
    /* Trivial path: already at destination — show completion state immediately */
    int finished = (path_length <= 1);

    Vector2 entity_position = positions[path[0]];

    Rectangle playButton = {20, 20, 120, 40};

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, playButton)) {
                if (path_length <= 1) {
                    /* Nothing to animate */
                } else if (finished) {
                    finished = 0;
                    current_edge_index = 0;
                    current_step = 0;
                    step_timer = 0.0f;
                    wait_timer = 0.0f;
                    is_waiting = 0;
                    is_playing = 1;
                    entity_position = positions[path[0]];
                } else {
                    is_playing = !is_playing;
                }
            }
        }

        if (is_playing && !finished && path_length > 1) {
            if (is_waiting) {
                wait_timer += delta;

                if (wait_timer >= NODE_WAIT_SEC) {
                    wait_timer = 0.0f;
                    is_waiting = 0;
                    current_step = 0;
                    step_timer = 0.0f;
                }
            } else {
                int from = path[current_edge_index];
                int to = path[current_edge_index + 1];
                int weight = edge_weight_between(g, from, to);

                step_timer += delta;

                if (step_timer >= EDGE_STEP_SEC) {
                    step_timer = 0.0f;
                    current_step++;

                    if (current_step >= weight) {
                        entity_position = positions[to];
                        current_step = 0;
                        current_edge_index++;

                        if (current_edge_index >= path_length - 1) {
                            finished = 1;
                            is_playing = 0;
                        } else {
                            is_waiting = 1;
                        }
                    } else {
                        float t = (float)current_step / (float)weight;
                        entity_position.x =
                            positions[from].x + t * (positions[to].x - positions[from].x);
                        entity_position.y =
                            positions[from].y + t * (positions[to].y - positions[from].y);
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int s = 0; s < node_num; s++) {
            edge* curr = g->adjacency_list[s];

            while (curr != NULL) {
                int d = curr->dest;
                int weight = curr->weight;

                Vector2 start = positions[s];
                Vector2 end = positions[d];

                float ang = atan2f(end.y - start.y, end.x - start.x);

                start.x += NODE_RADIUS * cosf(ang);
                start.y += NODE_RADIUS * sinf(ang);

                end.x -= NODE_RADIUS * cosf(ang);
                end.y -= NODE_RADIUS * sinf(ang);

                int on_path = edge_on_shortest_path(s, d, path, path_length);
                Color lineCol = on_path ? DARKGREEN : LIGHTGRAY;
                Color headCol = on_path ? GREEN : GRAY;
                float thick = on_path ? 4.0f : 2.0f;

                draw_arrow_colored(start, end, thick, lineCol, headCol);

                int midX = (int)((start.x + end.x) / 2);
                int midY = (int)((start.y + end.y) / 2);

                char weightText[32];
                const int fontSize = 20;
                snprintf(weightText, sizeof(weightText), "%d", weight);
                int tw = MeasureText(weightText, fontSize);
                DrawText(weightText, midX - tw / 2, midY - fontSize / 2, fontSize,
                         on_path ? DARKBLUE : BLUE);

                curr = curr->next;
            }
        }

        for (int i = 0; i < node_num; i++) {
            Color fill = RED;
            if (i == src) {
                fill = GREEN;
            } else if (i == dest) {
                fill = BLUE;
            }

            DrawCircleV(positions[i], NODE_RADIUS, fill);

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

        Color btnFill = LIGHTGRAY;
        if (path_length > 1) {
            btnFill = is_playing ? ORANGE : GREEN;
        }
        DrawRectangleRec(playButton, btnFill);
        DrawRectangleLinesEx(playButton, 2, DARKGRAY);
        const char* btnLabel = (path_length <= 1) ? "N/A" : (is_playing ? "STOP" : "PLAY");
        int labelFs = 20;
        int lw = MeasureText(btnLabel, labelFs);
        DrawText(btnLabel,
                 (int)(playButton.x + playButton.width / 2 - lw / 2),
                 (int)(playButton.y + playButton.height / 2 - labelFs / 2),
                 labelFs,
                 BLACK);

        DrawCircleV(entity_position, 12, PURPLE);

        if (finished) {
            DrawText("Arrived at destination!", 250, 30, 24, DARKGREEN);
            char distLine[48];
            snprintf(distLine, sizeof(distLine), "Shortest distance: %d", total_distance);
            int fs = 20;
            int tw = MeasureText(distLine, fs);
            DrawText(distLine, WIDTH / 2 - tw / 2, 62, fs, DARKGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    free_graph(g);

    return 0;
}
