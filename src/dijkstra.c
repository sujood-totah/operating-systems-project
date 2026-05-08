#include "dijkstra.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

void dijkstra(graph* g, int src, int dest) {
    if (g == NULL || src < 0 || dest < 0 || src >= g->node_num || dest >= g->node_num) {
        printf("Invalid input\n");
        return;
    }
    if (src == dest) {
        printf("%d\n", src);
        printf("0\n");
        return;
    }

    int* dist = malloc((sizeof(int)) * g->node_num);
    int* visited = malloc((sizeof(int)) *g->node_num);
    int* prev = malloc((sizeof(int)) *g->node_num);

    if (dist == NULL || visited == NULL || prev == NULL) {
        printf("Invalid input\n");
        free(dist);
        free(visited);
        free(prev);
        return;
    }

    for (int i = 0; i < g->node_num; i++) {
        dist[i] = INT_MAX;
        visited[i] = 0;
        prev[i] = -1;
    }
    dist[src] = 0;

    for (int i = 0; i < g->node_num; i++) {
        int min_distance = INT_MAX;
        int min_index = -1;
        for (int j = 0; j < g->node_num; j++) {
            if (visited[j] == 0 && dist[j] < min_distance) {
                min_distance = dist[j];
                min_index = j;
            }
        }
        if (min_index == -1) {
            break;
        }
        visited[min_index] = 1;

        edge* pointer = g->adjacency_list[min_index];
        while (pointer != NULL) {
            int neighbor = pointer->dest;
            int weight = pointer->weight;
            if (weight < 0) {
                printf("Invalid input\n");
                free(dist);
                free(visited);
                free(prev);
                return;
            }

            if (visited[neighbor] == 0 && dist[min_index] != INT_MAX && dist[min_index]+weight < dist[neighbor]) {
                dist[neighbor] = dist[min_index] + weight;
                prev[neighbor] = min_index;
            }
            pointer = pointer->next;
        }
    }

    if (dist[dest] == INT_MAX) {
        printf("No path found\n");
        free(dist);
        free(visited);
        free(prev);
        return;
    }

    int* path = malloc((sizeof(int)) * g->node_num);
    if (path == NULL) {
        printf("Invalid input\n");
        free(dist);
        free(visited);
        free(prev);
        return;
    }

    int path_length = 0;
    int current = dest;
    while (current != -1) {
        path[path_length] = current;
        path_length++;
        current = prev[current];
    }
    for (int i = path_length - 1; i >= 0; i--) {
        printf("%d", path[i]);
        if (i > 0) {
            printf(" -> ");
        }
    }

    printf("\n%d\n", dist[dest]);
    free(path);
    free(dist);
    free(visited);
    free(prev);
    return;

}
