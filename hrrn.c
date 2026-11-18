#include <stdio.h>
#include <stdlib.h>
#include "hrrn.h"

ResultSummary execute_hrrn(Process processes[], int n) {
    if (processes == NULL || n <= 0) {
        return (ResultSummary){0};
    }
    Process *p = malloc((size_t)n * sizeof(*p));
    GanttBlock *chart = malloc((size_t)(2 * n) * sizeof(*chart));
    int *completed = calloc((size_t)n, sizeof(*completed));
    if (p == NULL || chart == NULL || completed == NULL) {
        printf("Memory allocation failed for HRRN simulation.\n");
        free(p);
        free(chart);
        free(completed);
        return (ResultSummary){0};
    }
    for (int i = 0; i < n; i++) {
        p[i] = processes[i];
    }

    int currentTime = 0;
    int chartSize = 0;
    int completedCount = 0;
    while (completedCount < n) {
        int selected = -1;
        double highestRatio = -1.0;
        for (int i = 0; i < n; i++) {
            if (!completed[i] && p[i].arrivalTime <= currentTime) {
                int waiting = currentTime - p[i].arrivalTime;
                double ratio = (double)(waiting + p[i].burstTime) / p[i].burstTime;
                if (ratio > highestRatio ||
                    (ratio == highestRatio &&
                     (selected < 0 || p[i].arrivalTime < p[selected].arrivalTime ||
                      (p[i].arrivalTime == p[selected].arrivalTime &&
                       p[i].pid < p[selected].pid)))) {
                    selected = i;
                    highestRatio = ratio;
                }
            }
        }

        if (selected < 0) {
            int nextArrival = 1000000000;
            for (int i = 0; i < n; i++) {
                if (!completed[i] && p[i].arrivalTime < nextArrival) {
                    nextArrival = p[i].arrivalTime;
                }
            }
            chart[chartSize++] = (GanttBlock){-1, currentTime, nextArrival};
            currentTime = nextArrival;
            continue;
        }

        Process *current = &p[selected];
        current->firstStartTime = currentTime;
        current->responseTime = currentTime - current->arrivalTime;
        current->completionTime = currentTime + current->burstTime;
        current->turnaroundTime = current->completionTime - current->arrivalTime;
        current->waitingTime = current->turnaroundTime - current->burstTime;
        current->remainingTime = 0;
        chart[chartSize++] = (GanttBlock){current->pid, currentTime,
                                           current->completionTime};
        currentTime = current->completionTime;
        completed[selected] = 1;
        completedCount++;
    }

    ResultSummary result = print_and_calculate_metrics(
        "HRRN (Non-Preemptive)", p, n, chart, chartSize);
    free(p);
    free(chart);
    free(completed);
    return result;
}
