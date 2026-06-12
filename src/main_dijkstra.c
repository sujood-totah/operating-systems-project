#include <stdio.h>
#include <stdlib.h>

#include "graph_io.h"
#include "dijkstra.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Invalid input\n");
        return 1;
    }

    graph_load_data data;
    graph_load_result r = graph_load_milestone1_from_path(argv[1], &data);

    if (r == GRAPH_LOAD_NEGATIVE_WEIGHT) {
        printf("Invalid input\n");
        return 1;
    }
    if (r != GRAPH_LOAD_OK) {
        printf("Invalid input\n");
        return 1;
    }

    dijkstra(data.g, data.source[0], data.destination[0]);
    free(data.source);
    free(data.destination);
    free_graph(data.g);
    return 0;
}