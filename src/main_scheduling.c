#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "graph.h"
#include "graph_io.h"
#include "dijkstra.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

#include <semaphore.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "scheduler.h"


#define WIDTH 800
#define HEIGHT 600
#define NODE_RADIUS 25
#define GUI_MAX_NODES 15

#define STATE_WAITING_OUTSIDE 0
#define STATE_AT_NODE 1
#define STATE_FINISHED 2
#define STATE_REQUEST_NODE 3
#define STATE_RELEASE_NODE 4
#define WAITING_SLEEP_USEC 100000
#define EDGE_WAIT_RATIO 0.85f


typedef struct {
    sem_t node_sem[GUI_MAX_NODES];
} SharedNodeState;


typedef struct {
    pid_t pid;
    int traveler_id;
    int current_node;
    int next_node;
    int finished;
    int total_distance;
    int state;
    int waiting_for_node;
    int waiting_from_node;
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

    int state;
    int waiting_for_node;
    int waiting_from_node;
    int current_node;
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

static int parse_scheduler_args(int argc,
                                char* argv[],
                                SchedulerType* scheduler,
                                int* quantum,
                                const char** input_path) {
    if (argc < 4 || strcmp(argv[1], "-schd") != 0) {
        return 0;
    }

    if (strcmp(argv[2], "fcfs") == 0) {
        if (argc != 4) {
            return 0;
        }
        *scheduler = SCHED_FCFS;
        *quantum = 1;
        *input_path = argv[3];
        return 1;
    }

    if (strcmp(argv[2], "sjf") == 0) {
        if (argc != 4) {
            return 0;
        }
        *scheduler = SCHED_SJF;
        *quantum = 1;
        *input_path = argv[3];
        return 1;
    }

    if (strcmp(argv[2], "priority") == 0) {
        if (argc != 4) {
            return 0;
        }
        *scheduler = SCHED_PRIORITY;
        *quantum = 1;
        *input_path = argv[3];
        return 1;
    }

    if (strcmp(argv[2], "rr") == 0) {
        if (argc != 5) {
            return 0;
        }
        *scheduler = SCHED_RR;
        *quantum = atoi(argv[3]);
        if (*quantum <= 0) {
            return 0;
        }
        *input_path = argv[4];
        return 1;
    }

    return 0;
}

static void dispatch_next_at_node(int node,
                                NodeQueue* node_queues,
                                int* node_busy,
                                const pid_t* traveler_pids,
                                SchedulerType scheduler,
                                int quantum,
                                int* rr_remaining) {
    if (node_busy[node]) {
        return;
    }

    int selected = choose_next_traveler(&node_queues[node], scheduler, quantum);
    if (selected < 0) {
        return;
    }

    node_busy[node] = 1;
    if (scheduler == SCHED_RR) {
        rr_remaining[selected] = quantum - 1;
    }
    kill(traveler_pids[selected], SIGCONT);
}

static int get_edge_weight(graph* g, int from, int to) {
    int weight = 1;
    edge* curr = g->adjacency_list[from];
    while (curr != NULL) {
        if (curr->dest == to) {
            weight = curr->weight;
            break;
        }
        curr = curr->next;
    }
    return weight;
}

static void set_waiting_outside_visual(Traveler* traveler,
                                       const Vector2* positions,
                                       int waiting_from_node,
                                       int waiting_for_node,
                                       int next_node) {
    Vector2 from_pos = positions[waiting_from_node];
    Vector2 to_pos = positions[waiting_for_node];

    float dx = to_pos.x - from_pos.x;
    float dy = to_pos.y - from_pos.y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist == 0.0f && next_node >= 0) {
        Vector2 next_pos = positions[next_node];

        dx = next_pos.x - to_pos.x;
        dy = next_pos.y - to_pos.y;
        dist = sqrtf(dx * dx + dy * dy);

        if (dist > 0.0f) {
            float offset = NODE_RADIUS + 20.0f;
            traveler->position.x = to_pos.x + (dx / dist) * offset;
            traveler->position.y = to_pos.y + (dy / dist) * offset;
        }
    } else if (dist > 0.0f) {
        traveler->position.x = from_pos.x + dx * EDGE_WAIT_RATIO;
        traveler->position.y = from_pos.y + dy * EDGE_WAIT_RATIO;
    }
}

static void apply_semaphore_waiting(Traveler* traveler,
                                    graph* g,
                                    const Vector2* positions,
                                    const Message* msg) {
    int from = msg->waiting_from_node;
    int to = msg->waiting_for_node;

    traveler->waiting_for_node = to;
    traveler->waiting_from_node = from;

    if (from != to) {
        for (int k = 0; k < traveler->path_length - 1; k++) {
            if (traveler->path[k] == from && traveler->path[k + 1] == to) {
                traveler->current_edge_index = k;
                break;
            }
        }

        int weight = get_edge_weight(g, from, to);
        int waiting_step = (int)(EDGE_WAIT_RATIO * (float)weight);
        if (waiting_step < 1) {
            waiting_step = 1;
        }
        if (waiting_step >= weight) {
            waiting_step = weight - 1;
        }
        if (weight > 1) {
            traveler->current_step = waiting_step;
        }
    }

    set_waiting_outside_visual(traveler, positions, from, to, msg->next_node);
}

static void advance_traveler_animation(Traveler* traveler,
                                       graph* g,
                                       const Vector2* positions,
                                       float delta) {
    if (traveler->finished || traveler->path_length <= 1) {
        return;
    }

    if (traveler->state == STATE_WAITING_OUTSIDE) {
        return;
    }

    if (traveler->is_waiting) {
        traveler->wait_timer += delta;

        if (traveler->wait_timer >= 1.0f) {
            traveler->wait_timer = 0.0f;
            traveler->is_waiting = 0;
        }
        return;
    }

    int from = traveler->path[traveler->current_edge_index];
    int to = traveler->path[traveler->current_edge_index + 1];
    int weight = get_edge_weight(g, from, to);

    traveler->step_timer += delta;

    if (traveler->step_timer >= 0.3f) {
        traveler->step_timer = 0.0f;
        traveler->current_step++;

        if (traveler->current_step >= weight) {
            traveler->position = positions[to];
            traveler->current_step = 0;
            traveler->current_edge_index++;

            if (traveler->current_edge_index >= traveler->path_length - 1) {
                traveler->finished = 1;
            } else {
                traveler->is_waiting = 1;
            }
        } else {
            float t = (float)traveler->current_step / (float)weight;
            traveler->position.x =
                positions[from].x + t * (positions[to].x - positions[from].x);
            traveler->position.y =
                positions[from].y + t * (positions[to].y - positions[from].y);
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
    SchedulerType scheduler;
    int quantum = 1;
    const char* input_path = NULL;

    if (!parse_scheduler_args(argc, argv, &scheduler, &quantum, &input_path)) {
        printf("Usage: ./sim -schd <fcfs|sjf|priority|rr> [quantum] <input_file>\n");
        return 1;
    }

    graph_load_data data;
    graph_load_result r = graph_load_from_path(input_path, &data, GUI_MAX_NODES);

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
        travelers[i].path_length = 0;

        if (!dijkstra_get_path(g,
                               src[i],
                               dest[i],
                               travelers[i].path,
                               &travelers[i].path_length,
                               &travelers[i].total_distance)) {
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
        travelers[i].state = STATE_AT_NODE;
        travelers[i].waiting_for_node = -1;
        travelers[i].waiting_from_node = -1;
        travelers[i].current_node = travelers[i].path[0];

        travelers[i].position = positions[travelers[i].path[0]];
    }

    key_t key;
    key = ftok("/tmp", 'm');

    if (key == -1) {
        perror("ftok failed");
        exit(EXIT_FAILURE);
    }

    int shm_id;
    shm_id = shmget(key, sizeof(SharedNodeState), IPC_CREAT | 0600);

    if (shm_id == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    SharedNodeState * shm_ptr;
    shm_ptr = (SharedNodeState *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < GUI_MAX_NODES; i++) {
        sem_init(&shm_ptr->node_sem[i], 1, 1);
    }

    int pipes[traveler_count][2];
    NodeQueue node_queues[GUI_MAX_NODES];
    int node_busy[GUI_MAX_NODES] = {0};
    pid_t traveler_pids[traveler_count];
    int rr_remaining[traveler_count];
    int arrival_counter = 0;

    for (int q = 0; q < GUI_MAX_NODES; q++) {
        init_queue(&node_queues[q]);
    }

    for (int i = 0; i < traveler_count; i++) {
        rr_remaining[i] = 0;
    }

    for (int i = 0; i < traveler_count; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            return 1;
        }
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

                int target_node = child_path[j];
                int from_node;

                if (j == 0) {
                    from_node = target_node;
                } else {
                    from_node = child_path[j - 1];
                }

                msg.pid = getpid();
                msg.traveler_id = i;
                msg.current_node = target_node;
                msg.waiting_for_node = target_node;
                msg.waiting_from_node = from_node;
                msg.total_distance = child_total_distance;

                if (j == child_path_length - 1) {
                    msg.next_node = -1;
                    msg.finished = 1;
                } else {
                    msg.next_node = child_path[j + 1];
                    msg.finished = 0;
                }

                msg.state = STATE_REQUEST_NODE;
                msg.finished = 0;
                write(pipes[i][1], &msg, sizeof(Message));
                raise(SIGSTOP);

                while (sem_trywait(&shm_ptr->node_sem[target_node]) == -1) {
                    msg.state = STATE_WAITING_OUTSIDE;
                    msg.finished = 0;
                    write(pipes[i][1], &msg, sizeof(Message));
                    usleep(WAITING_SLEEP_USEC);
                }

                msg.state = STATE_AT_NODE;
                msg.finished = 0;
                write(pipes[i][1], &msg, sizeof(Message));
                sleep(1);

                sem_post(&shm_ptr->node_sem[target_node]);

                msg.state = STATE_RELEASE_NODE;
                msg.finished = 0;
                write(pipes[i][1], &msg, sizeof(Message));

                if (j == child_path_length - 1) {
                    msg.state = STATE_FINISHED;
                    msg.finished = 1;
                    write(pipes[i][1], &msg, sizeof(Message));
                }
            }

            close(pipes[i][1]);
            exit(0);
        }

        close(pipes[i][1]);
        fcntl(pipes[i][0], F_SETFL, O_NONBLOCK);
        travelers[i].pid = pid;
        traveler_pids[i] = pid;
    }

    printf("Scheduler: %s", scheduler_name(scheduler));
    if (scheduler == SCHED_RR) {
        printf(" (quantum=%d)", quantum);
    }
    printf("\n");

    InitWindow(WIDTH, HEIGHT, "Graph GUI");
    SetTargetFPS(60);
    Rectangle playButton = {20, 20, 120, 40};
    int is_playing = 0;
    int has_started = 0;


    while (!WindowShouldClose()) {
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
                advance_traveler_animation(&travelers[i], g, positions, delta);
            }
        }

        Message msg;
        for (int i = 0; i < traveler_count; i++) {

            ssize_t bytes = read(pipes[i][0], &msg, sizeof(Message));

            if (bytes > 0) {
                travelers[msg.traveler_id].waiting_for_node = msg.waiting_for_node;
                travelers[msg.traveler_id].waiting_from_node = msg.waiting_from_node;
                travelers[msg.traveler_id].current_node = msg.current_node;
                travelers[msg.traveler_id].total_distance = msg.total_distance;

                if (msg.state == STATE_REQUEST_NODE) {
                    int node = msg.waiting_for_node;
                    int burst_time = msg.total_distance;
                    int priority = msg.traveler_id;
                    int arrival_order = arrival_counter++;

                    travelers[msg.traveler_id].state = STATE_WAITING_OUTSIDE;
                    apply_semaphore_waiting(
                        &travelers[msg.traveler_id],
                        g,
                        positions,
                        &msg
                    );

                    if (scheduler == SCHED_RR && rr_remaining[msg.traveler_id] > 0) {
                        prepend_to_queue(&node_queues[node],
                                           msg.traveler_id,
                                           node,
                                           burst_time,
                                           priority,
                                           arrival_order);
                        rr_remaining[msg.traveler_id]--;
                    } else {
                        add_to_queue(&node_queues[node],
                                     msg.traveler_id,
                                     node,
                                     burst_time,
                                     priority,
                                     arrival_order);
                    }

                    dispatch_next_at_node(node,
                                          node_queues,
                                          node_busy,
                                          traveler_pids,
                                          scheduler,
                                          quantum,
                                          rr_remaining);

                    printf("[PID=%d] requested node %d\n", msg.pid, node);

                } else if (msg.state == STATE_WAITING_OUTSIDE) {
                    travelers[msg.traveler_id].state = STATE_WAITING_OUTSIDE;
                    apply_semaphore_waiting(
                        &travelers[msg.traveler_id],
                        g,
                        positions,
                        &msg
                    );

                    printf("[PID=%d] waiting outside node %d\n",
                           msg.pid,
                           msg.waiting_for_node);

                } else if (msg.state == STATE_AT_NODE) {
                    travelers[msg.traveler_id].state = STATE_AT_NODE;

                    printf("[PID=%d] arrived at node %d | next node: %d\n",
                           msg.pid,
                           msg.current_node,
                           msg.next_node);

                } else if (msg.state == STATE_RELEASE_NODE) {
                    int node = msg.current_node;

                    node_busy[node] = 0;
                    dispatch_next_at_node(node,
                                          node_queues,
                                          node_busy,
                                          traveler_pids,
                                          scheduler,
                                          quantum,
                                          rr_remaining);

                    printf("[PID=%d] released node %d\n", msg.pid, node);

                } else if (msg.state == STATE_FINISHED) {
                    travelers[msg.traveler_id].child_finished = 1;

                    printf("[PID=%d] arrived at node %d | DESTINATION\n",
                           msg.pid,
                           msg.current_node);

                    printf("[PID=%d] finished\n", msg.pid);
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

        for (int t = 0; t < traveler_count; t++) {
            if (travelers[t].state == STATE_WAITING_OUTSIDE) {
                DrawCircleLines(
                    (int)travelers[t].position.x,
                    (int)travelers[t].position.y,
                    15,
                    ORANGE
                );

                DrawText(
                    "W",
                    (int)travelers[t].position.x - 6,
                    (int)travelers[t].position.y - 8,
                    18,
                    ORANGE
                );
            } else {
                DrawCircleV(
                    travelers[t].position,
                    12,
                    travelers[t].color
                );
            }

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

    for (int i = 0; i < GUI_MAX_NODES; i++) {
        sem_destroy(&shm_ptr->node_sem[i]);
    }

    shmdt(shm_ptr);
    shmctl(shm_id, IPC_RMID, NULL);

    free(data.source);
    free(data.destination);
    free_graph(g);

    return 0;
}
