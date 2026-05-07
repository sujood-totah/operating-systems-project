#include <stdio.h>
#include <stdlib.h>
#include "graph.h"
#include "dijkstra.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("ERROR: Wrong number of arguments\n");
        exit(1);
    }
    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("error opening file");
        exit(1);
    }
    int node_num;
    int edge_num;

    fscanf(file, "%d %d", &node_num, &edge_num);
    graph* g = create_graph(node_num);

    for (int i = 0; i < edge_num; i++) {
        int src;
        int dest;
        int weight;

        fscanf(file, "%d %d %d", &src, &dest, &weight);
        add_edge(g,src,dest,weight);
    }
    int source, destination;
    fscanf(file, "%d %d", &source,&destination);
    dijkstra(g, source, destination);
    fclose(file);
    free_graph(g);
    return 0;
}
