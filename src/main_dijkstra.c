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
    graph_load_result r = graph_load_from_path(argv[1], &data, 0);

    if (r == GRAPH_LOAD_NEGATIVE_WEIGHT) {
        printf("Negative weights are not allowed.\n");
        return 1;
    }
    if (r != GRAPH_LOAD_OK) {
        printf("Invalid input\n");
        return 1;
    }
    int traveler_count = data.traveler_count;


    for (int i = 0; i < traveler_count; i++) {
        dijkstra(data.g, data.source[i], data.destination[i]);
    }
    free(data.source);
    free(data.destination);
    free_graph(data.g);
    return 0;
}