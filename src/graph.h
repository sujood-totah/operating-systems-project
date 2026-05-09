#ifndef GRAPH_H
#define GRAPH_H

typedef struct edge {
    int dest;
    int weight;
    struct edge* next;
}edge;

typedef struct graph {
    int node_num;
    edge** adjacency_list;
}graph;

graph* create_graph(int node_num);
/* Returns 0 on success, -1 if allocation fails. */
int add_edge(graph* graph, int src, int dest, int weight);
void free_graph(graph* graph);

#endif
