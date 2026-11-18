#include <stdio.h>
#include <stdlib.h>
#include "priority_np.h"

ResultSummary execute_priority_np(Process processes[], int n) {
    if (processes == NULL || n <= 0) {
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    Process *p = (Process *)malloc(n * sizeof(Process));
    if (p == NULL) {
        printf("Memory allocation failed for Priority (NP) copy!\n");
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    for (int i = 0; i < n; i++) p[i] = processes[i];
    
    GanttBlock *chart = (GanttBlock *)malloc(2 * n * sizeof(GanttBlock));
    if (chart == NULL) {
        printf("Memory allocation failed for Gantt chart!\n");
        free(p);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    int chartSize = 0;
    
    int *isCompleted = (int *)calloc(n, sizeof(int));
    if (isCompleted == NULL) {
        printf("Memory allocation failed for completion array!\n");
        free(p);
        free(chart);
        ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
        return empty;
    }
    
    int currentTime = 0;
    int completedCount = 0;
    
    while (completedCount < n) {
        int selectedProcess = -1;
        int minPriority = 999999999;
        
        for (int i = 0; i < n; i++) {
            if (p[i].arrivalTime <= currentTime && !isCompleted[i]) {
                if (p[i].priority < minPriority) {
                    minPriority = p[i].priority;
                    selectedProcess = i;
                } else if (p[i].priority == minPriority) {
                    if (p[i].arrivalTime < p[selectedProcess].arrivalTime) {
                        selectedProcess = i;
                    } else if (p[i].arrivalTime == p[selectedProcess].arrivalTime) {
                        if (p[i].pid < p[selectedProcess].pid) {
                            selectedProcess = i;
                        }
                    }
                }
            }
        }
        
        if (selectedProcess == -1) {
            int nextArrival = 999999999;
            for (int i = 0; i < n; i++) {
                if (!isCompleted[i] && p[i].arrivalTime < nextArrival) {
                    nextArrival = p[i].arrivalTime;
                }
            }
            
            if (nextArrival == 999999999) {
                free(p);
                free(chart);
                free(isCompleted);
                ResultSummary empty = {"", 0, 0, 0, 0, 0, 0};
                return empty;
            }
            
            chart[chartSize].pid = -1;
            chart[chartSize].startTime = currentTime;
            chart[chartSize].endTime = nextArrival;
            chartSize++;
            
            currentTime = nextArrival;
        } else {
            int i = selectedProcess;
            p[i].firstStartTime = currentTime;
            p[i].responseTime = p[i].firstStartTime - p[i].arrivalTime;
            
            chart[chartSize].pid = p[i].pid;
            chart[chartSize].startTime = currentTime;
            
            currentTime += p[i].burstTime;
            
            chart[chartSize].endTime = currentTime;
            chartSize++;
            
            p[i].completionTime = currentTime;
            p[i].turnaroundTime = p[i].completionTime - p[i].arrivalTime;
            p[i].waitingTime = p[i].turnaroundTime - p[i].burstTime;
            p[i].remainingTime = 0;
            
            isCompleted[i] = 1;
            completedCount++;
        }
    }
    
    ResultSummary res = print_and_calculate_metrics("Priority (Non-Preemptive)", p, n, chart, chartSize);
    
    free(isCompleted);
    free(p);
    free(chart);
    return res;
}
