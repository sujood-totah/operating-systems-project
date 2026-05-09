#include "graph.h"
#include <stdlib.h>

graph* create_graph(int node_num) {
    graph* g = (graph*)malloc(sizeof(graph));
    if (g == NULL) {
        return NULL;
    }
    g->node_num = node_num;
    g->adjacency_list = malloc((size_t)node_num * sizeof(edge*));
    if (g->adjacency_list == NULL) {
        free(g);
        return NULL;
    }
    for (int i = 0; i < node_num; i++) {
        g->adjacency_list[i] = NULL;
    }
    return g;
}



int add_edge(graph* graph, int src, int dest, int weight) {
    edge* new_edge = (edge*)malloc(sizeof(edge));
    if (new_edge == NULL) {
        return -1;
    }
    new_edge->dest = dest;
    new_edge->weight = weight;
    new_edge->next = graph->adjacency_list[src];
    graph->adjacency_list[src] = new_edge;
    return 0;
}


void free_graph(graph* graph) {
    for (int i = 0; i < graph->node_num; i++) {
        edge * current = graph->adjacency_list[i];
        edge * next;
        while (current != NULL) {
            next = current->next;
            free(current);
            current = next;
        }
    }
    free(graph->adjacency_list);
    free(graph);
}
