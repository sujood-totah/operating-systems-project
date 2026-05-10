#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph.h"

void dijkstra(graph* g, int src, int dest);

int dijkstra_get_path(graph* g, int src, int dest, int* path, int* path_length, int* total_distance);

#endif