#ifndef GRAPH_IO_H
#define GRAPH_IO_H

#include "graph.h"

typedef enum {
    GRAPH_LOAD_OK = 0,
    GRAPH_LOAD_INVALID,
    GRAPH_LOAD_NEGATIVE_WEIGHT,
    GRAPH_LOAD_ALLOC
} graph_load_result;

typedef struct {
    graph* g;
    int traveler_count;
    int* source;
    int* destination;
} graph_load_data;

/*
 * Reads assignment graph file format into *out.
 * max_nodes: if > 0, rejects when N > max_nodes (GUI cap); if 0, no cap (CLI).
 * On success, caller must free_graph(out->g).
 */
graph_load_result graph_load_from_path(const char* path, graph_load_data* out,
                                       int max_nodes);

/* Milestone 1: last line is "source dest" (no traveler-count line). */
graph_load_result graph_load_milestone1_from_path(const char* path,
                                                  graph_load_data* out);

#endif
