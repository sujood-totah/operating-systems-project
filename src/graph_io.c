#include "graph_io.h"
#include <stdio.h>

graph_load_result graph_load_from_path(const char* path, graph_load_data* out,
                                       int max_nodes) {
    out->g = NULL;
    out->source = 0;
    out->destination = 0;

    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return GRAPH_LOAD_INVALID;
    }

    int node_num, edge_num;
    if (fscanf(file, "%d %d", &node_num, &edge_num) != 2 ||
        node_num <= 0 || edge_num < 0) {
        fclose(file);
        return GRAPH_LOAD_INVALID;
        }
    if (max_nodes > 0 && node_num > max_nodes) {
        fclose(file);
        return GRAPH_LOAD_INVALID;
    }

    graph* g = create_graph(node_num);
    if (g == NULL) {
        fclose(file);
        return GRAPH_LOAD_ALLOC;
    }

    for (int i = 0; i < edge_num; i++) {
        int src, dest, weight;
        if (fscanf(file, "%d %d %d", &src, &dest, &weight) != 3 ||
            src < 0 || dest < 0 ||
            src >= node_num || dest >= node_num) {
            fclose(file);
            free_graph(g);
            return GRAPH_LOAD_INVALID;
            }
        if (weight < 0) {
            fclose(file);
            free_graph(g);
            return GRAPH_LOAD_NEGATIVE_WEIGHT;
        }
        if (add_edge(g, src, dest, weight) != 0) {
            fclose(file);
            free_graph(g);
            return GRAPH_LOAD_ALLOC;
        }
    }

    int source, destination;
    if (fscanf(file, "%d %d", &source, &destination) != 2 ||
        source < 0 || destination < 0 ||
        source >= node_num || destination >= node_num) {
        fclose(file);
        free_graph(g);
        return GRAPH_LOAD_INVALID;
        }

    fclose(file);
    out->g = g;
    out->source = source;
    out->destination = destination;
    return GRAPH_LOAD_OK;
}

