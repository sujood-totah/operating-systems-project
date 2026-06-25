#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_WAITING 100

typedef enum {
    SCHED_FCFS,
    SCHED_SJF,
    SCHED_PRIORITY,
    SCHED_RR
} SchedulerType;

typedef struct {
    int traveler_id;
    int node_id;
    int burst_time;
    int priority;
    int arrival_order;
} WaitingTraveler;

typedef struct {
    WaitingTraveler items[MAX_WAITING];
    int count;
} NodeQueue;

void init_queue(NodeQueue* queue);

int is_queue_empty(NodeQueue* queue);

int add_to_queue(NodeQueue* queue,
                 int traveler_id,
                 int node_id,
                 int burst_time,
                 int priority,
                 int arrival_order);

int prepend_to_queue(NodeQueue* queue,
                     int traveler_id,
                     int node_id,
                     int burst_time,
                     int priority,
                     int arrival_order);

int choose_next_traveler(NodeQueue* queue,
                         SchedulerType scheduler,
                         int quantum);

const char* scheduler_name(SchedulerType scheduler);

#endif
