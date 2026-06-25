#define _POSIX_C_SOURCE 200809L
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "graph.h"
#include "graph_io.h"
#include "dijkstra.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

#define WIDTH 800
#define HEIGHT 600
#define NODE_RADIUS 25
#define GUI_MAX_NODES 15

typedef struct {
    pid_t pid;
    int traveler_id;
    int current_node;
    int next_node;
    int finished;
    int total_distance;
} Message;

typedef struct {
    int source;
    int destination;

    int path[GUI_MAX_NODES];
    int path_length;
    int total_distance;

    int current_edge_index;
    int current_step;

    float step_timer;
    float wait_timer;
    int is_waiting;

    int finished;
    int child_finished;
    Vector2 position;
    Color color;
    pid_t pid;
} Traveler;

static volatile sig_atomic_t child_can_run = 0;

static void handle_start_signal(int signum) {
    (void)signum;
    child_can_run = 1;
}

static void signal_active_travelers(const Traveler* travelers,
                                    int traveler_count,
                                    int signal_num) {
    for (int i = 0; i < traveler_count; i++) {
        if (!travelers[i].finished && !travelers[i].child_finished && travelers[i].pid > 0) {
            kill(travelers[i].pid, signal_num);
        }
    }
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
        printf("Negative weights are not allowed.\n");
        return 1;
    }
    if (r != GRAPH_LOAD_OK) {
        printf("Invalid input\n");
        return 1;
    }

    int traveler_count = data.traveler_count;
    graph* g = data.g;
    const int node_num = g->node_num;

    const int* src = data.source;
    const int* dest = data.destination;
    // int path[GUI_MAX_NODES];
    // int path_length = 0;
    // int total_distance = 0;

    Traveler travelers[traveler_count];

    Color colors[] = {
        RED,
        BLUE,
        GREEN,
        ORANGE,
        PURPLE,
        PINK,
        BROWN,
        YELLOW
    };

    Vector2 positions[GUI_MAX_NODES];

    float centerX = WIDTH / 2.0f;
    float centerY = HEIGHT / 2.0f;
    float layoutRadius = 200.0f;

    for (int i = 0; i < node_num; i++) {
        float angle = 2 * PI * i / node_num;
        positions[i].x = centerX + layoutRadius * cosf(angle);
        positions[i].y = centerY + layoutRadius * sinf(angle);
    }

    for (int i = 0; i < traveler_count; i++) {
        travelers[i].source = src[i];
        travelers[i].destination = dest[i];

        int has_path = dijkstra_get_path(
            g,
            src[i],
            dest[i],
            travelers[i].path,
            &travelers[i].path_length,
            &travelers[i].total_distance
        );
        if (!has_path) {
            printf("No path found\n");
            free(data.source);
            free(data.destination);
            free_graph(g);
            return 1;
        }

        travelers[i].position = positions[travelers[i].source];
        travelers[i].color = colors[i % 8];

        travelers[i].current_edge_index = 0;
        travelers[i].current_step = 0;
        travelers[i].step_timer = 0.0f;
        travelers[i].wait_timer = 0.0f;
        travelers[i].is_waiting = 0;
        travelers[i].finished = (travelers[i].path_length <= 1);
        travelers[i].child_finished = 0;

        travelers[i].position = positions[travelers[i].path[0]];
    }

    int pipes[traveler_count][2];
    for (int i = 0; i < traveler_count; i++) {
        pipe(pipes[i]);
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            free_graph(g);
            return 1;
        }

        if (pid == 0) {
            close(pipes[i][0]);

            int child_path[GUI_MAX_NODES];
            int child_path_length = 0;
            int child_total_distance = 0;
            struct sigaction sa;

            int has_path = dijkstra_get_path(
                           g,
                           src[i],
                           dest[i],
                           child_path,
                           &child_path_length,
                           &child_total_distance
                       );
            if (!has_path) {
                close(pipes[i][1]);
                exit(1);
            }

            child_can_run = 0;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sa.sa_handler = handle_start_signal;
            sigaction(SIGUSR1, &sa, NULL);

            while (!child_can_run) {
                pause();
            }

            Message msg;

            for (int j = 0; j < child_path_length; j++) {

                msg.pid = getpid();
                msg.traveler_id = i;
                msg.current_node = child_path[j];
                msg.total_distance = child_total_distance;

                if (j == child_path_length - 1) {
                    msg.next_node = -1;
                    msg.finished = 1;
                } else {
                    msg.next_node = child_path[j + 1];
                    msg.finished = 0;
                }

                write(pipes[i][1], &msg, sizeof(Message));

                if (!msg.finished) {
                    sleep(1);
                }
            }

            close(pipes[i][1]);
            exit(0);
        }

        close(pipes[i][1]);
        fcntl(pipes[i][0], F_SETFL, O_NONBLOCK);
        travelers[i].pid = pid;
    }



    InitWindow(WIDTH, HEIGHT, "Graph GUI");
    SetTargetFPS(60);
    Rectangle playButton = {20, 20, 120, 40};
    int is_playing = 0;
    int has_started = 0;


    while (!WindowShouldClose()) {
        // button paly/stop
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();

            if (CheckCollisionPointRec(mouse, playButton)) {
                if (!has_started) {
                    signal_active_travelers(travelers, traveler_count, SIGUSR1);
                    has_started = 1;
                    is_playing = 1;
                } else if (is_playing) {
                    signal_active_travelers(travelers, traveler_count, SIGSTOP);
                    is_playing = 0;
                } else {
                    signal_active_travelers(travelers, traveler_count, SIGCONT);
                    is_playing = 1;
                }
            }
        }

        if (is_playing) {
            float delta = GetFrameTime();

            for (int i = 0; i < traveler_count; i++) {
                if (travelers[i].finished || travelers[i].path_length <= 1) {
                    continue;
                }

                if (travelers[i].is_waiting) {
                    travelers[i].wait_timer += delta;

                    if (travelers[i].wait_timer >= 1.0f) {
                        travelers[i].wait_timer = 0.0f;
                        travelers[i].is_waiting = 0;
                    }
                } else {
                    int from = travelers[i].path[travelers[i].current_edge_index];
                    int to = travelers[i].path[travelers[i].current_edge_index + 1];

                    int weight = 1;
                    edge* curr = g->adjacency_list[from];
                    while (curr != NULL) {
                        if (curr->dest == to) {
                            weight = curr->weight;
                            break;
                        }
                        curr = curr->next;
                    }

                    travelers[i].step_timer += delta;

                    if (travelers[i].step_timer >= 0.3f) {
                        travelers[i].step_timer = 0.0f;
                        travelers[i].current_step++;

                        if (travelers[i].current_step >= weight) {
                            travelers[i].position = positions[to];
                            travelers[i].current_step = 0;
                            travelers[i].current_edge_index++;

                            if (travelers[i].current_edge_index >= travelers[i].path_length - 1) {
                                travelers[i].finished = 1;
                            } else {
                                travelers[i].is_waiting = 1;
                            }
                        } else {
                            float t = (float)travelers[i].current_step / (float)weight;
                            travelers[i].position.x =
                                positions[from].x + t * (positions[to].x - positions[from].x);
                            travelers[i].position.y =
                                positions[from].y + t * (positions[to].y - positions[from].y);
                        }
                    }
                }
            }
        }

        Message msg;
        for (int i = 0; i < traveler_count; i++) {

            ssize_t bytes = read(pipes[i][0], &msg, sizeof(Message));

            if (bytes > 0) {
                travelers[msg.traveler_id].total_distance = msg.total_distance;

                if (msg.finished) {
                    travelers[msg.traveler_id].child_finished = 1;

                    printf("[PID=%d] arrived at node %d | DESTINATION\n",
                           msg.pid,
                           msg.current_node);

                    printf("[PID=%d] finished\n", msg.pid);

                } else {
                    printf("[PID=%d] arrived at node %d | next node: %d\n",
                           msg.pid,
                           msg.current_node,
                           msg.next_node);
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        /* Draw edges */
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

                int on_path = 0;

                for (int t = 0; t < traveler_count; t++) {
                    if (edge_on_shortest_path(
                            s,
                            d,
                            travelers[t].path,
                            travelers[t].path_length
                        )) {
                        on_path = 1;
                        break;
                    }
                }

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
                DrawText(
                    weightText,
                    midX - tw / 2,
                    midY - fontSize / 2,
                    fontSize,
                    on_path ? DARKBLUE : BLUE
                    );

                curr = curr->next;
            }
        }

        /* Draw nodes */
        for (int i = 0; i < node_num; i++) {
            Color fill = RED;

            for (int t = 0; t < traveler_count; t++) {
                if (i == travelers[t].source) {
                    fill = GREEN;
                }
                if (i == travelers[t].destination) {
                    fill = BLUE;
                }
            }

            DrawCircleV(positions[i], NODE_RADIUS, fill);

            char nodeText[16];
            const int fontSize = 20;
            snprintf(nodeText, sizeof(nodeText), "%d", i);

            int tw = MeasureText(nodeText, fontSize);
            DrawText(
                nodeText,
                (int)(positions[i].x - tw / 2),
                (int)(positions[i].y - fontSize / 2),
                fontSize,
                WHITE
            );
        }

        /* Draw play/stop button */
        Color btnFill = is_playing ? ORANGE : GREEN;

        DrawRectangleRec(playButton, btnFill);
        DrawRectangleLinesEx(playButton, 2, DARKGRAY);

        const char* btnLabel = is_playing ? "STOP" : "PLAY";

        int labelFs = 20;
        int lw = MeasureText(btnLabel, labelFs);

        DrawText(
            btnLabel,
            (int)(playButton.x + playButton.width / 2 - lw / 2),
            (int)(playButton.y + playButton.height / 2 - labelFs / 2),
            labelFs,
            BLACK
        );

        /* Draw all travelers */
        for (int t = 0; t < traveler_count; t++) {
            DrawCircleV(
                travelers[t].position,
                12,
                travelers[t].color
            );

            char label[16];
            snprintf(label, sizeof(label), "T%d", t);

            DrawText(
                label,
                (int)travelers[t].position.x - 10,
                (int)travelers[t].position.y - 30,
                16,
                BLACK
            );
        }

        /* Draw status text */
        int all_finished = 1;

        for (int t = 0; t < traveler_count; t++) {
            if (!travelers[t].finished) {
                all_finished = 0;
                break;
            }
        }

        if (all_finished) {
            DrawText("All travelers arrived!", 260, 30, 24, DARKGREEN);
        }

        for (int t = 0; t < traveler_count; t++) {
            char line[128];

            snprintf(
                line,
                sizeof(line),
                "T%d: %d -> %d | distance: %d%s",
                t,
                travelers[t].source,
                travelers[t].destination,
                travelers[t].total_distance,
                travelers[t].finished ? " | finished" : ""
            );

            DrawText(line, 20, 80 + t * 22, 18, DARKGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    for (int i = 0; i < traveler_count; i++) {
        if (!travelers[i].child_finished) {
            kill(travelers[i].pid, SIGCONT);
            kill(travelers[i].pid, SIGTERM);
        }
    }


    for (int i = 0; i < traveler_count; i++) {
        waitpid(travelers[i].pid, NULL, 0);
    }
    free(data.source);
    free(data.destination);
    free_graph(g);

    return 0;
}