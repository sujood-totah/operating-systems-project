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
                 int priority,
                 int arrival_order) {
    if (queue->count >= MAX_WAITING) {
        return 0;
    }

    queue->items[queue->count].traveler_id = traveler_id;
    queue->items[queue->count].node_id = node_id;
    queue->items[queue->count].burst_time = burst_time;
    queue->items[queue->count].priority = priority;
    queue->items[queue->count].arrival_order = arrival_order;

    queue->count++;
    return 1;
}

int prepend_to_queue(NodeQueue* queue,
                     int traveler_id,
                     int node_id,
                     int burst_time,
                     int priority,
                     int arrival_order) {
    if (queue->count >= MAX_WAITING) {
        return 0;
    }

    for (int i = queue->count; i > 0; i--) {
        queue->items[i] = queue->items[i - 1];
    }

    queue->items[0].traveler_id = traveler_id;
    queue->items[0].node_id = node_id;
    queue->items[0].burst_time = burst_time;
    queue->items[0].priority = priority;
    queue->items[0].arrival_order = arrival_order;

    queue->count++;
    return 1;
}

static int find_best_index(const NodeQueue* queue,
                           int (*better)(const WaitingTraveler*,
                                         const WaitingTraveler*)) {
    int selected_index = 0;

    for (int i = 1; i < queue->count; i++) {
        if (better(&queue->items[i], &queue->items[selected_index])) {
            selected_index = i;
        }
    }

    return selected_index;
}

static int fcfs_better(const WaitingTraveler* candidate,
                       const WaitingTraveler* current) {
    return candidate->arrival_order < current->arrival_order;
}

static int sjf_better(const WaitingTraveler* candidate,
                      const WaitingTraveler* current) {
    if (candidate->burst_time != current->burst_time) {
        return candidate->burst_time < current->burst_time;
    }
    return candidate->arrival_order < current->arrival_order;
}

static int priority_better(const WaitingTraveler* candidate,
                           const WaitingTraveler* current) {
    if (candidate->priority != current->priority) {
        return candidate->priority < current->priority;
    }
    return candidate->arrival_order < current->arrival_order;
}

static int remove_at_index(NodeQueue* queue, int selected_index) {
    int traveler_id = queue->items[selected_index].traveler_id;

    for (int i = selected_index; i < queue->count - 1; i++) {
        queue->items[i] = queue->items[i + 1];
    }

    queue->count--;
    return traveler_id;
}

int choose_next_traveler(NodeQueue* queue,
                         SchedulerType scheduler,
                         int quantum) {
    (void)quantum;

    if (queue->count == 0) {
        return -1;
    }

    int selected_index = 0;

    if (scheduler == SCHED_FCFS) {
        selected_index = find_best_index(queue, fcfs_better);
    } else if (scheduler == SCHED_SJF) {
        selected_index = find_best_index(queue, sjf_better);
    } else if (scheduler == SCHED_PRIORITY) {
        selected_index = find_best_index(queue, priority_better);
    } else if (scheduler == SCHED_RR) {
        selected_index = 0;
    }

    return remove_at_index(queue, selected_index);
}

const char* scheduler_name(SchedulerType scheduler) {
    switch (scheduler) {
        case SCHED_FCFS:
            return "FCFS";
        case SCHED_SJF:
            return "SJF";
        case SCHED_PRIORITY:
            return "Priority";
        case SCHED_RR:
            return "Round Robin";
        default:
            return "Unknown";
    }
}
