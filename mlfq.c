#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "mlfq.h"

typedef struct {
    int *data;
    int front;
    int rear;
    int size;
    int capacity;
} MLFQQueue;

static int init_queue(MLFQQueue *q, int capacity) {
    q->data = (int *)malloc((size_t)capacity * sizeof(int));
    if (q->data == NULL) {
        return 0;
    }
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->capacity = capacity;
    return 1;
}

static void free_queue(MLFQQueue *q) {
    if (q->data != NULL) {
        free(q->data);
        q->data = NULL;
    }
    q->size = 0;
    q->capacity = 0;
}

static int is_empty(const MLFQQueue *q) {
    return q->size == 0;
}

static int push_queue(MLFQQueue *q, int val) {
    if (q->size >= q->capacity) {
        int newCap = q->capacity * 2;
        int *newData = (int *)malloc((size_t)newCap * sizeof(int));
        if (newData == NULL) {
            return 0;
        }
        for (int i = 0; i < q->size; i++) {
            newData[i] = q->data[(q->front + i) % q->capacity];
        }
        free(q->data);
        q->data = newData;
        q->front = 0;
        q->rear = q->size;
        q->capacity = newCap;
    }
    q->data[q->rear] = val;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
    return 1;
}

static int pop_queue(MLFQQueue *q) {
    if (q->size == 0) {
        return -1;
    }
    int val = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return val;
}

static int peek_queue(const MLFQQueue *q) {
    if (q->size == 0) {
        return -1;
    }
    return q->data[q->front];
}

static int ensure_chart_capacity(GanttBlock **chartPtr, int *capacityPtr, int size) {
    if (size >= *capacityPtr) {
        if (*capacityPtr > 10000000) {
            return 0;
        }
        int newCapacity = (*capacityPtr) * 2;
        GanttBlock *temp = (GanttBlock *)realloc(*chartPtr, (size_t)newCapacity * sizeof(GanttBlock));
        if (temp == NULL) {
            return 0;
        }
        *chartPtr = temp;
        *capacityPtr = newCapacity;
    }
    return 1;
}

ResultSummary execute_mlfq(Process processes[], int n) {
    if (processes == NULL || n <= 0) {
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }

    Process *p = (Process *)malloc((size_t)n * sizeof(Process));
    if (p == NULL) {
        printf("Memory allocation failed for MLFQ copy!\n");
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    for (int i = 0; i < n; i++) {
        p[i] = processes[i];
    }

    // Sort by arrival time ascending, then PID ascending
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (p[j].arrivalTime > p[j + 1].arrivalTime ||
               (p[j].arrivalTime == p[j + 1].arrivalTime && p[j].pid > p[j + 1].pid)) {
                Process temp = p[j];
                p[j] = p[j + 1];
                p[j + 1] = temp;
            }
        }
    }

    int chartCapacity = 1000;
    GanttBlock *chart = (GanttBlock *)malloc((size_t)chartCapacity * sizeof(GanttBlock));
    if (chart == NULL) {
        printf("Memory allocation failed for Gantt chart!\n");
        free(p);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    int chartSize = 0;

    MLFQQueue q1, q2, q3;
    int initialQueueCap = n + 5;
    if (!init_queue(&q1, initialQueueCap) ||
        !init_queue(&q2, initialQueueCap) ||
        !init_queue(&q3, initialQueueCap)) {
        printf("Memory allocation failed for MLFQ queues!\n");
        free_queue(&q1);
        free_queue(&q2);
        free_queue(&q3);
        free(chart);
        free(p);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }

    int *timeSpentInQueue = (int *)calloc((size_t)n, sizeof(int));
    int *hasArrived = (int *)calloc((size_t)n, sizeof(int));
    if (timeSpentInQueue == NULL || hasArrived == NULL) {
        printf("Memory allocation failed for MLFQ tracking!\n");
        free_queue(&q1);
        free_queue(&q2);
        free_queue(&q3);
        free(timeSpentInQueue);
        free(hasArrived);
        free(chart);
        free(p);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }

    int currentTime = 0;
    int completedCount = 0;
    int prevPid = -2;

    while (completedCount < n) {
        // 1. Admit all newly arrived processes into Queue 1
        for (int i = 0; i < n; i++) {
            if (!hasArrived[i] && p[i].arrivalTime <= currentTime) {
                if (!push_queue(&q1, i)) {
                    printf("Failed to enqueue process into MLFQ Queue 1!\n");
                    free_queue(&q1); free_queue(&q2); free_queue(&q3);
                    free(timeSpentInQueue); free(hasArrived);
                    free(chart); free(p);
                    return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
                }
                hasArrived[i] = 1;
            }
        }

        // 2. If all queues are empty, CPU is idle
        if (is_empty(&q1) && is_empty(&q2) && is_empty(&q3)) {
            int nextArrival = INT_MAX;
            for (int i = 0; i < n; i++) {
                if (!hasArrived[i] && p[i].arrivalTime < nextArrival) {
                    nextArrival = p[i].arrivalTime;
                }
            }
            if (nextArrival == INT_MAX) {
                break;
            }
            if (prevPid != -1) {
                if (!ensure_chart_capacity(&chart, &chartCapacity, chartSize)) {
                    printf("Memory reallocation failed for Gantt chart!\n");
                    free_queue(&q1); free_queue(&q2); free_queue(&q3);
                    free(timeSpentInQueue); free(hasArrived);
                    free(chart); free(p);
                    return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
                }
                chart[chartSize].pid = -1;
                chart[chartSize].startTime = currentTime;
                chart[chartSize].endTime = nextArrival;
                chartSize++;
                prevPid = -1;
            } else {
                chart[chartSize - 1].endTime = nextArrival;
            }
            currentTime = nextArrival;

            // Admit newly arrived processes at nextArrival
            for (int i = 0; i < n; i++) {
                if (!hasArrived[i] && p[i].arrivalTime <= currentTime) {
                    if (!push_queue(&q1, i)) {
                        printf("Failed to enqueue process into MLFQ Queue 1!\n");
                        free_queue(&q1); free_queue(&q2); free_queue(&q3);
                        free(timeSpentInQueue); free(hasArrived);
                        free(chart); free(p);
                        return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
                    }
                    hasArrived[i] = 1;
                }
            }
        }

        // 3. Select process from highest priority non-empty queue
        int activeQueue = 0;
        int currIdx = -1;
        if (!is_empty(&q1)) {
            activeQueue = 1;
            currIdx = peek_queue(&q1);
        } else if (!is_empty(&q2)) {
            activeQueue = 2;
            currIdx = peek_queue(&q2);
        } else if (!is_empty(&q3)) {
            activeQueue = 3;
            currIdx = peek_queue(&q3);
        }

        if (currIdx < 0) {
            continue;
        }

        // Record first start time / response time
        if (p[currIdx].firstStartTime == -1) {
            p[currIdx].firstStartTime = currentTime;
            p[currIdx].responseTime = p[currIdx].firstStartTime - p[currIdx].arrivalTime;
        }

        // Record or extend Gantt block
        if (prevPid != p[currIdx].pid) {
            if (!ensure_chart_capacity(&chart, &chartCapacity, chartSize)) {
                printf("Memory reallocation failed for Gantt chart!\n");
                free_queue(&q1); free_queue(&q2); free_queue(&q3);
                free(timeSpentInQueue); free(hasArrived);
                free(chart); free(p);
                return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
            }
            chart[chartSize].pid = p[currIdx].pid;
            chart[chartSize].startTime = currentTime;
            chart[chartSize].endTime = currentTime + 1;
            chartSize++;
            prevPid = p[currIdx].pid;
        } else {
            chart[chartSize - 1].endTime = currentTime + 1;
        }

        // Execute for 1 time unit
        p[currIdx].remainingTime--;
        timeSpentInQueue[currIdx]++;
        currentTime++;

        // Check if process finished or exhausted its quantum
        if (p[currIdx].remainingTime == 0) {
            if (activeQueue == 1) {
                pop_queue(&q1);
            } else if (activeQueue == 2) {
                pop_queue(&q2);
            } else {
                pop_queue(&q3);
            }
            p[currIdx].completionTime = currentTime;
            p[currIdx].turnaroundTime = p[currIdx].completionTime - p[currIdx].arrivalTime;
            p[currIdx].waitingTime = p[currIdx].turnaroundTime - p[currIdx].burstTime;
            completedCount++;
        } else {
            // Quantum demotion
            if (activeQueue == 1 && timeSpentInQueue[currIdx] == 2) {
                pop_queue(&q1);
                timeSpentInQueue[currIdx] = 0;
                if (!push_queue(&q2, currIdx)) {
                    printf("Failed to demote process to MLFQ Queue 2!\n");
                    free_queue(&q1); free_queue(&q2); free_queue(&q3);
                    free(timeSpentInQueue); free(hasArrived);
                    free(chart); free(p);
                    return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
                }
            } else if (activeQueue == 2 && timeSpentInQueue[currIdx] == 4) {
                pop_queue(&q2);
                timeSpentInQueue[currIdx] = 0;
                if (!push_queue(&q3, currIdx)) {
                    printf("Failed to demote process to MLFQ Queue 3!\n");
                    free_queue(&q1); free_queue(&q2); free_queue(&q3);
                    free(timeSpentInQueue); free(hasArrived);
                    free(chart); free(p);
                    return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
                }
            }
        }
    }

    ResultSummary res = print_and_calculate_metrics(
        "MLFQ (Q1=2, Q2=4, Q3=FCFS)", p, n, chart, chartSize);

    free_queue(&q1);
    free_queue(&q2);
    free_queue(&q3);
    free(timeSpentInQueue);
    free(hasArrived);
    free(chart);
    free(p);

    return res;
}
