#include <stdio.h>
#include <stdlib.h>
#include "graph.h"
#include "dijkstra.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Invalid input\n");
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Invalid input\n");
        return 1;
    }

    int node_num, edge_num;

    if (fscanf(file, "%d %d", &node_num, &edge_num) != 2 ||
        node_num <= 0 || edge_num < 0) {
        printf("Invalid input\n");
        fclose(file);
        return 1;
        }

    graph* g = create_graph(node_num);
    if (g == NULL) {
        printf("Invalid input\n");
        fclose(file);
        return 1;
    }

    for (int i = 0; i < edge_num; i++) {
        int src, dest, weight;

        if (fscanf(file, "%d %d %d", &src, &dest, &weight) != 3 ||
            src < 0 || dest < 0 || weight < 0 ||
            src >= node_num || dest >= node_num) {
            printf("Invalid input\n");
            fclose(file);
            free_graph(g);
            return 1;
            }

        add_edge(g, src, dest, weight);
    }

    int source, destination;

    if (fscanf(file, "%d %d", &source, &destination) != 2 ||
        source < 0 || destination < 0 ||
        source >= node_num || destination >= node_num) {
        printf("Invalid input\n");
        fclose(file);
        free_graph(g);
        return 1;
        }

    dijkstra(g, source, destination);

    fclose(file);
    free_graph(g);
    return 0;
}