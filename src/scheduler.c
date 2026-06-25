#include "scheduler.h"

void init_queue(NodeQueue* queue) {
    queue->count = 0;
}

int is_queue_empty(NodeQueue* queue) {
    return queue->count == 0;
}

int add_to_queue(NodeQueue* queue,
                 int traveler_id,
                 int node_id,
                 int burst_time,
                 int arrival_order) {
    if (queue->count >= MAX_WAITING) {
        return 0;
    }

    queue->items[queue->count].traveler_id = traveler_id;
    queue->items[queue->count].node_id = node_id;
    queue->items[queue->count].burst_time = burst_time;
    queue->items[queue->count].arrival_order = arrival_order;

    queue->count++;
    return 1;
}

int choose_next_traveler(NodeQueue* queue,
                         SchedulerType scheduler) {
    if (queue->count == 0) {
        return -1;
    }

    int selected_index = 0;

    if (scheduler == SCHED_FCFS) {
        selected_index = 0;
    }
    else if (scheduler == SCHED_SJF) {
        for (int i = 1; i < queue->count; i++) {
            if (queue->items[i].burst_time <
                queue->items[selected_index].burst_time) {
                selected_index = i;
                }
        }
    }

    int traveler_id = queue->items[selected_index].traveler_id;

    for (int i = selected_index; i < queue->count - 1; i++) {
        queue->items[i] = queue->items[i + 1];
    }

    queue->count--;

    return traveler_id;
}