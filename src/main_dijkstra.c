#include <stdio.h>
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
        printf("Invalid input\n");
        return 1;
    }
    if (r != GRAPH_LOAD_OK) {
        printf("Invalid input\n");
        return 1;
    }

    dijkstra(data.g, data.source, data.destination);
    free_graph(data.g);
    return 0;
}
