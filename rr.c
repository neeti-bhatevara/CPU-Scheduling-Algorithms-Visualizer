#include <stdio.h>
#include <stdlib.h>
#include "rr.h"

static int ensure_chart_capacity(GanttBlock **chartPtr, int *capacityPtr, int size) {
    if (size >= *capacityPtr) {
        if (*capacityPtr > 10000000) {
            return 0;
        }
        int newCapacity = (*capacityPtr) * 2;
        GanttBlock *temp = (GanttBlock *)realloc(*chartPtr, newCapacity * sizeof(GanttBlock));
        if (temp == NULL) {
            return 0;
        }
        *chartPtr = temp;
        *capacityPtr = newCapacity;
    }
    return 1;
}

static int enqueue_process(int *queue, int *rear, int front, int capacity, int processIdx) {
    int next = (*rear + 1) % capacity;
    if (next == front) {
        return 0; // Queue is full
    }
    queue[*rear] = processIdx;
    *rear = next;
    return 1;
}

static int dequeue_process(int *queue, int *front, int capacity) {
    int processIdx = queue[*front];
    *front = (*front + 1) % capacity;
    return processIdx;
}

ResultSummary execute_rr(Process processes[], int n, int quantum) {
    if (processes == NULL || n <= 0 || quantum <= 0) {
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    Process *p = (Process *)malloc(n * sizeof(Process));
    if (p == NULL) {
        printf("Memory allocation failed for Round Robin copy!\n");
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    for (int i = 0; i < n; i++) p[i] = processes[i];
    
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (p[j].arrivalTime > p[j + 1].arrivalTime || 
               (p[j].arrivalTime == p[j + 1].arrivalTime && p[j].pid > p[j + 1].pid)) {
                Process temp = p[j]; p[j] = p[j + 1]; p[j + 1] = temp;
            }
        }
    }

    int chartCapacity = 1000;
    GanttBlock *chart = (GanttBlock *)malloc(chartCapacity * sizeof(GanttBlock));
    if (chart == NULL) {
        printf("Memory allocation failed for Gantt chart!\n");
        free(p);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    int chartSize = 0;
    
    if (n > 10000000) {
        printf("Process count exceeds safe limit for Round Robin!\n");
        free(p);
        free(chart);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    int queueCapacity = n + 5;
    int *queue = (int *)malloc(queueCapacity * sizeof(int));
    if (queue == NULL) {
        printf("Memory allocation failed for queue!\n");
        free(p);
        free(chart);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    int front = 0, rear = 0;
    
    int *inQueue = (int *)calloc(n, sizeof(int));
    if (inQueue == NULL) {
        printf("Memory allocation failed for inQueue tracking!\n");
        free(p);
        free(chart);
        free(queue);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    
    int currentTime = 0;
    int completedCount = 0;
    int prevPid = -2;
    
    if (p[0].arrivalTime > 0) {
        currentTime = p[0].arrivalTime;
        if (!ensure_chart_capacity(&chart, &chartCapacity, chartSize)) {
            printf("Memory reallocation failed for Gantt chart!\n");
            free(p); free(chart); free(queue); free(inQueue);
            ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
            return empty;
        }
        chart[chartSize].pid = -1;
        chart[chartSize].startTime = 0;
        chart[chartSize].endTime = currentTime;
        chartSize++;
        prevPid = -1;
    }
    
    for (int i = 0; i < n; i++) {
        if (p[i].arrivalTime <= currentTime && !inQueue[i]) {
            if (!enqueue_process(queue, &rear, front, queueCapacity, i)) {
                printf("Error: Round Robin queue overflow!\n");
                free(p); free(chart); free(queue); free(inQueue);
                return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
            }
            inQueue[i] = 1;
        }
    }
    
    while (completedCount < n) {
        if (front == rear) {
            int nextArrival = 999999999;
            for (int i = 0; i < n; i++) {
                if (p[i].remainingTime > 0 && p[i].arrivalTime < nextArrival) {
                    nextArrival = p[i].arrivalTime;
                }
            }
            if (prevPid != -1) {
                if (!ensure_chart_capacity(&chart, &chartCapacity, chartSize)) {
                    printf("Memory reallocation failed for Gantt chart!\n");
                    free(p); free(chart); free(queue); free(inQueue);
                    ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
                    return empty;
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
            
            for (int i = 0; i < n; i++) {
                if (p[i].arrivalTime <= currentTime && p[i].remainingTime > 0 && !inQueue[i]) {
                    if (!enqueue_process(queue, &rear, front, queueCapacity, i)) {
                        printf("Error: Round Robin queue overflow!\n");
                        free(p); free(chart); free(queue); free(inQueue);
                        return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
                    }
                    inQueue[i] = 1;
                }
            }
        } else {
            int currentProcess = dequeue_process(queue, &front, queueCapacity);
            
            if (p[currentProcess].firstStartTime == -1) {
                p[currentProcess].firstStartTime = currentTime;
                p[currentProcess].responseTime = p[currentProcess].firstStartTime - p[currentProcess].arrivalTime;
            }
            
            int executionTime = p[currentProcess].remainingTime < quantum ? p[currentProcess].remainingTime : quantum;
            
            if (prevPid != p[currentProcess].pid) {
                if (!ensure_chart_capacity(&chart, &chartCapacity, chartSize)) {
                    printf("Memory reallocation failed for Gantt chart!\n");
                    free(p); free(chart); free(queue); free(inQueue);
                    ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
                    return empty;
                }
                chart[chartSize].pid = p[currentProcess].pid;
                chart[chartSize].startTime = currentTime;
                chart[chartSize].endTime = currentTime + executionTime;
                chartSize++;
                prevPid = p[currentProcess].pid;
            } else {
                chart[chartSize - 1].endTime = currentTime + executionTime;
            }
            
            p[currentProcess].remainingTime -= executionTime;
            currentTime += executionTime;
            
            for (int i = 0; i < n; i++) {
                if (p[i].arrivalTime > currentTime - executionTime && p[i].arrivalTime <= currentTime && p[i].remainingTime > 0 && !inQueue[i]) {
                    if (!enqueue_process(queue, &rear, front, queueCapacity, i)) {
                        printf("Error: Round Robin queue overflow!\n");
                        free(p); free(chart); free(queue); free(inQueue);
                        return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
                    }
                    inQueue[i] = 1;
                }
            }
            
            if (p[currentProcess].remainingTime > 0) {
                if (!enqueue_process(queue, &rear, front, queueCapacity, currentProcess)) {
                    printf("Error: Round Robin queue overflow!\n");
                    free(p); free(chart); free(queue); free(inQueue);
                    return (ResultSummary){"", 0, 0, 0, 0, 0, 0};
                }
            } else {
                p[currentProcess].completionTime = currentTime;
                p[currentProcess].turnaroundTime = p[currentProcess].completionTime - p[currentProcess].arrivalTime;
                p[currentProcess].waitingTime = p[currentProcess].turnaroundTime - p[currentProcess].burstTime;
                completedCount++;
            }
        }
    }
    
    char title[50];
    sprintf(title, "Round Robin (Q=%d)", quantum);
    ResultSummary res = print_and_calculate_metrics(title, p, n, chart, chartSize);
    
    free(p);
    free(chart);
    free(queue);
    free(inQueue);
    return res;
}
